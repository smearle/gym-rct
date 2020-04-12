/*****************************************************************************
 * Copyright (c) 2014-2019 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "Viewport.h"

#include "../Context.h"
#include "../Game.h"
#include "../Input.h"
#include "../OpenRCT2.h"
#include "../config/Config.h"
#include "../core/Guard.hpp"
#include "../core/JobPool.hpp"
#include "../drawing/Drawing.h"
#include "../paint/Paint.h"
#include "../peep/Staff.h"
#include "../ride/Ride.h"
#include "../ride/TrackDesign.h"
#include "../ui/UiContext.h"
#include "../ui/WindowManager.h"
#include "../world/Climate.h"
#include "../world/Map.h"
#include "../world/Sprite.h"
#include "Colour.h"
#include "Window.h"
#include "Window_internal.h"

#include <algorithm>
#include <cstring>

using namespace OpenRCT2;

//#define DEBUG_SHOW_DIRTY_BOX
uint8_t gShowGridLinesRefCount;
uint8_t gShowLandRightsRefCount;
uint8_t gShowConstuctionRightsRefCount;

rct_viewport g_viewport_list[MAX_VIEWPORT_COUNT];
rct_viewport* g_music_tracking_viewport;

static std::unique_ptr<JobPool> _paintJobs;

ScreenCoordsXY gSavedView;
ZoomLevel gSavedViewZoom;
uint8_t gSavedViewRotation;

paint_entry* gNextFreePaintStruct;
uint8_t gCurrentRotation;

static uint32_t _currentImageType;
InteractionInfo::InteractionInfo(const paint_struct* ps)
    : Loc(ps->map_x, ps->map_y)
    , Element(ps->tileElement)
    , SpriteType(ps->sprite_type)
{
}
static void viewport_paint_weather_gloom(rct_drawpixelinfo* dpi);

/**
 * This is not a viewport function. It is used to setup many variables for
 * multiple things.
 *  rct2: 0x006E6EAC
 */
void viewport_init_all()
{
    if (!gOpenRCT2NoGraphics)
    {
        colours_init_maps();
    }

    window_init_all();

    // Setting up viewports
    for (int32_t i = 0; i < MAX_VIEWPORT_COUNT; i++)
    {
        g_viewport_list[i].width = 0;
    }

    // ?
    input_reset_flags();
    input_set_state(INPUT_STATE_RESET);
    gPressedWidget.window_classification = 255;
    gPickupPeepImage = UINT32_MAX;
    reset_tooltip_not_shown();
    gMapSelectFlags = 0;
    gStaffDrawPatrolAreas = 0xFFFF;
    textinput_cancel();
}

/**
 * Converts between 3d point of a sprite to 2d coordinates for centring on that
 * sprite
 *  rct2: 0x006EB0C1
 * x : ax
 * y : bx
 * z : cx
 * out_x : ax
 * out_y : bx
 */
std::optional<ScreenCoordsXY> centre_2d_coordinates(const CoordsXYZ& loc, rct_viewport* viewport)
{
    // If the start location was invalid
    // propagate the invalid location to the output.
    // This fixes a bug that caused the game to enter an infinite loop.
    if (loc.isNull())
    {
        return std::nullopt;
    }

    auto screenCoord = translate_3d_to_2d_with_z(get_current_rotation(), loc);
    screenCoord.x -= viewport->view_width / 2;
    screenCoord.y -= viewport->view_height / 2;
    return { screenCoord };
}

/**
 * Viewport will look at sprite or at coordinates as specified in flags 0b_1X
 * for sprite 0b_0X for coordinates
 *
 *  rct2: 0x006EB009
 *  x:      ax
 *  y:      eax (top 16)
 *  width:  bx
 *  height: ebx (top 16)
 *  zoom:   cl (8 bits)
 *  centre_x: edx lower 16 bits
 *  centre_y: edx upper 16 bits
 *  centre_z: ecx upper 16 bits
 *  sprite: edx lower 16 bits
 *  flags:  edx top most 2 bits 0b_X1 for zoom clear see below for 2nd bit.
 *  w:      esi
 */
void viewport_create(
    rct_window* w, const ScreenCoordsXY& screenCoords, int32_t width, int32_t height, int32_t zoom, CoordsXYZ centrePos,
    char flags, uint16_t sprite)
{
    rct_viewport* viewport = nullptr;
    for (int32_t i = 0; i < MAX_VIEWPORT_COUNT; i++)
    {
        if (g_viewport_list[i].width == 0)
        {
            viewport = &g_viewport_list[i];
            break;
        }
    }
    if (viewport == nullptr)
    {
        log_error("No more viewport slots left to allocate.");
        return;
    }

    viewport->pos = screenCoords;
    viewport->width = width;
    viewport->height = height;

    if (!(flags & VIEWPORT_FOCUS_TYPE_COORDINATE))
    {
        zoom = 0;
    }

    viewport->view_width = width << zoom;
    viewport->view_height = height << zoom;
    viewport->zoom = zoom;
    viewport->flags = 0;

    if (gConfigGeneral.always_show_gridlines)
        viewport->flags |= VIEWPORT_FLAG_GRIDLINES;
    w->viewport = viewport;

    if (flags & VIEWPORT_FOCUS_TYPE_SPRITE)
    {
        w->viewport_target_sprite = sprite;
        rct_sprite* centre_sprite = get_sprite(sprite);
        centrePos = { centre_sprite->generic.x, centre_sprite->generic.y, centre_sprite->generic.z };
    }
    else
    {
        w->viewport_target_sprite = SPRITE_INDEX_NULL;
    }

    auto centreLoc = centre_2d_coordinates(centrePos, viewport);
    if (!centreLoc)
    {
        log_error("Invalid location for viewport.");
        return;
    }
    w->savedViewPos = *centreLoc;
    viewport->viewPos = *centreLoc;
}

/**
 *
 *  rct2: 0x00689174
 * edx is assumed to be (and always is) the current rotation, so it is not
 * needed as parameter.
 */
CoordsXYZ viewport_adjust_for_map_height(const ScreenCoordsXY& startCoords)
{
    int16_t height = 0;

    uint32_t rotation = get_current_rotation();
    CoordsXY pos{};
    for (int32_t i = 0; i < 6; i++)
    {
        pos = viewport_coord_to_map_coord(startCoords.x, startCoords.y, height);
        height = tile_element_height(pos);

        // HACK: This is to prevent the x and y values being set to values outside
        // of the map. This can happen when the height is larger than the map size.
        int16_t max = gMapSizeMinus2;
        if (pos.x > max && pos.y > max)
        {
            const CoordsXY corr[] = { { -1, -1 }, { 1, -1 }, { 1, 1 }, { -1, 1 } };
            pos.x += corr[rotation].x * height;
            pos.y += corr[rotation].y * height;
        }
    }

    return { pos, height };
}

/*
 *  rct2: 0x006E7FF3
 */
static void viewport_redraw_after_shift(
    rct_drawpixelinfo* dpi, rct_window* window, rct_viewport* viewport, int32_t x, int32_t y)
{
    // sub-divide by intersecting windows
    if (window != nullptr)
    {
        // skip current window and non-intersecting windows
        if (viewport == window->viewport || viewport->pos.x + viewport->width <= window->windowPos.x
            || viewport->pos.x >= window->windowPos.x + window->width
            || viewport->pos.y + viewport->height <= window->windowPos.y
            || viewport->pos.y >= window->windowPos.y + window->height)
        {
            auto itWindowPos = window_get_iterator(window);
            auto itNextWindow = itWindowPos != g_window_list.end() ? std::next(itWindowPos) : g_window_list.end();
            viewport_redraw_after_shift(
                dpi, itNextWindow == g_window_list.end() ? nullptr : itNextWindow->get(), viewport, x, y);
            return;
        }

        // save viewport
        rct_viewport view_copy = *viewport;

        if (viewport->pos.x < window->windowPos.x)
        {
            viewport->width = window->windowPos.x - viewport->pos.x;
            viewport->view_width = viewport->width * viewport->zoom;
            viewport_redraw_after_shift(dpi, window, viewport, x, y);

            viewport->pos.x += viewport->width;
            viewport->viewPos.x += viewport->width * viewport->zoom;
            viewport->width = view_copy.width - viewport->width;
            viewport->view_width = viewport->width * viewport->zoom;
            viewport_redraw_after_shift(dpi, window, viewport, x, y);
        }
        else if (viewport->pos.x + viewport->width > window->windowPos.x + window->width)
        {
            viewport->width = window->windowPos.x + window->width - viewport->pos.x;
            viewport->view_width = viewport->width * viewport->zoom;
            viewport_redraw_after_shift(dpi, window, viewport, x, y);

            viewport->pos.x += viewport->width;
            viewport->viewPos.x += viewport->width * viewport->zoom;
            viewport->width = view_copy.width - viewport->width;
            viewport->view_width = viewport->width * viewport->zoom;
            viewport_redraw_after_shift(dpi, window, viewport, x, y);
        }
        else if (viewport->pos.y < window->windowPos.y)
        {
            viewport->height = window->windowPos.y - viewport->pos.y;
            viewport->view_width = viewport->width * viewport->zoom;
            viewport_redraw_after_shift(dpi, window, viewport, x, y);

            viewport->pos.y += viewport->height;
            viewport->viewPos.y += viewport->height * viewport->zoom;
            viewport->height = view_copy.height - viewport->height;
            viewport->view_width = viewport->width * viewport->zoom;
            viewport_redraw_after_shift(dpi, window, viewport, x, y);
        }
        else if (viewport->pos.y + viewport->height > window->windowPos.y + window->height)
        {
            viewport->height = window->windowPos.y + window->height - viewport->pos.y;
            viewport->view_width = viewport->width * viewport->zoom;
            viewport_redraw_after_shift(dpi, window, viewport, x, y);

            viewport->pos.y += viewport->height;
            viewport->viewPos.y += viewport->height * viewport->zoom;
            viewport->height = view_copy.height - viewport->height;
            viewport->view_width = viewport->width * viewport->zoom;
            viewport_redraw_after_shift(dpi, window, viewport, x, y);
        }

        // restore viewport
        *viewport = view_copy;
    }
    else
    {
        int16_t left = viewport->pos.x;
        int16_t right = viewport->pos.x + viewport->width;
        int16_t top = viewport->pos.y;
        int16_t bottom = viewport->pos.y + viewport->height;

        // if moved more than the viewport size
        if (abs(x) < viewport->width && abs(y) < viewport->height)
        {
            // update whole block ?
            drawing_engine_copy_rect(viewport->pos.x, viewport->pos.y, viewport->width, viewport->height, x, y);

            if (x > 0)
            {
                // draw left
                int16_t _right = viewport->pos.x + x;
                window_draw_all(dpi, left, top, _right, bottom);
                left += x;
            }
            else if (x < 0)
            {
                // draw right
                int16_t _left = viewport->pos.x + viewport->width + x;
                window_draw_all(dpi, _left, top, right, bottom);
                right += x;
            }

            if (y > 0)
            {
                // draw top
                bottom = viewport->pos.y + y;
                window_draw_all(dpi, left, top, right, bottom);
            }
            else if (y < 0)
            {
                // draw bottom
                top = viewport->pos.y + viewport->height + y;
                window_draw_all(dpi, left, top, right, bottom);
            }
        }
        else
        {
            // redraw whole viewport
            window_draw_all(dpi, left, top, right, bottom);
        }
    }
}

static void viewport_shift_pixels(
    rct_drawpixelinfo* dpi, rct_window* window, rct_viewport* viewport, int16_t x_diff, int16_t y_diff)
{
    auto it = window_get_iterator(window);
    for (; it != g_window_list.end(); it++)
    {
        auto w = it->get();
        if (!(w->flags & WF_TRANSPARENT))
            continue;
        if (w->viewport == viewport)
            continue;

        if (viewport->pos.x + viewport->width <= w->windowPos.x)
            continue;
        if (w->windowPos.x + w->width <= viewport->pos.x)
            continue;

        if (viewport->pos.y + viewport->height <= w->windowPos.y)
            continue;
        if (w->windowPos.y + w->height <= viewport->pos.y)
            continue;

        auto left = w->windowPos.x;
        auto right = w->windowPos.x + w->width;
        auto top = w->windowPos.y;
        auto bottom = w->windowPos.y + w->height;

        if (left < viewport->pos.x)
            left = viewport->pos.x;
        if (right > viewport->pos.x + viewport->width)
            right = viewport->pos.x + viewport->width;

        if (top < viewport->pos.y)
            top = viewport->pos.y;
        if (bottom > viewport->pos.y + viewport->height)
            bottom = viewport->pos.y + viewport->height;

        if (left >= right)
            continue;
        if (top >= bottom)
            continue;

        window_draw_all(dpi, left, top, right, bottom);
    }

    viewport_redraw_after_shift(dpi, window, viewport, x_diff, y_diff);
}

static void viewport_move(int16_t x, int16_t y, rct_window* w, rct_viewport* viewport)
{
    auto zoom = viewport->zoom;

    // Note: do not do the subtraction and then divide!
    // Note: Due to arithmetic shift != /zoom a shift will have to be used
    // hopefully when 0x006E7FF3 is finished this can be converted to /zoom.
    int16_t x_diff = (viewport->viewPos.x / viewport->zoom) - (x / viewport->zoom);
    int16_t y_diff = (viewport->viewPos.y / viewport->zoom) - (y / viewport->zoom);

    viewport->viewPos = { x, y };

    // If no change in viewing area
    if ((!x_diff) && (!y_diff))
        return;

    if (w->flags & WF_7)
    {
        int32_t left = std::max<int32_t>(viewport->pos.x, 0);
        int32_t top = std::max<int32_t>(viewport->pos.y, 0);
        int32_t right = std::min<int32_t>(viewport->pos.x + viewport->width, context_get_width());
        int32_t bottom = std::min<int32_t>(viewport->pos.y + viewport->height, context_get_height());

        if (left >= right)
            return;
        if (top >= bottom)
            return;

        if (drawing_engine_has_dirty_optimisations())
        {
            rct_drawpixelinfo* dpi = drawing_engine_get_dpi();
            window_draw_all(dpi, left, top, right, bottom);
            return;
        }
    }

    rct_viewport view_copy = *viewport;

    if (viewport->pos.x < 0)
    {
        viewport->width += viewport->pos.x;
        viewport->view_width += viewport->pos.x * zoom;
        viewport->viewPos.x -= viewport->pos.x * zoom;
        viewport->pos.x = 0;
    }

    int32_t eax = viewport->pos.x + viewport->width - context_get_width();
    if (eax > 0)
    {
        viewport->width -= eax;
        viewport->view_width -= eax * zoom;
    }

    if (viewport->width <= 0)
    {
        *viewport = view_copy;
        return;
    }

    if (viewport->pos.y < 0)
    {
        viewport->height += viewport->pos.y;
        viewport->view_height += viewport->pos.y * zoom;
        viewport->viewPos.y -= viewport->pos.y * zoom;
        viewport->pos.y = 0;
    }

    eax = viewport->pos.y + viewport->height - context_get_height();
    if (eax > 0)
    {
        viewport->height -= eax;
        viewport->view_height -= eax * zoom;
    }

    if (viewport->height <= 0)
    {
        *viewport = view_copy;
        return;
    }

    if (drawing_engine_has_dirty_optimisations())
    {
        rct_drawpixelinfo* dpi = drawing_engine_get_dpi();
        viewport_shift_pixels(dpi, w, viewport, x_diff, y_diff);
    }

    *viewport = view_copy;
}

// rct2: 0x006E7A15
static void viewport_set_underground_flag(int32_t underground, rct_window* window, rct_viewport* viewport)
{
    if (window->classification != WC_MAIN_WINDOW)
    {
        if (!underground)
        {
            int32_t bit = viewport->flags & VIEWPORT_FLAG_UNDERGROUND_INSIDE;
            viewport->flags &= ~VIEWPORT_FLAG_UNDERGROUND_INSIDE;
            if (!bit)
                return;
        }
        else
        {
            int32_t bit = viewport->flags & VIEWPORT_FLAG_UNDERGROUND_INSIDE;
            viewport->flags |= VIEWPORT_FLAG_UNDERGROUND_INSIDE;
            if (bit)
                return;
        }
        window->Invalidate();
    }
}

/**
 *
 *  rct2: 0x006E7A3A
 */
void viewport_update_position(rct_window* window)
{
    window_event_resize_call(window);

    rct_viewport* viewport = window->viewport;
    if (!viewport)
        return;

    if (window->viewport_smart_follow_sprite != SPRITE_INDEX_NULL)
    {
        viewport_update_smart_sprite_follow(window);
    }

    if (window->viewport_target_sprite != SPRITE_INDEX_NULL)
    {
        viewport_update_sprite_follow(window);
        return;
    }

    viewport_set_underground_flag(0, window, viewport);

    int16_t x = window->savedViewPos.x + viewport->view_width / 2;
    int16_t y = window->savedViewPos.y + viewport->view_height / 2;

    auto mapCoord = viewport_coord_to_map_coord(x, y, 0);

    // Clamp to the map minimum value
    int32_t at_map_edge = 0;
    if (mapCoord.x < MAP_MINIMUM_X_Y)
    {
        mapCoord.x = MAP_MINIMUM_X_Y;
        at_map_edge = 1;
    }
    if (mapCoord.y < MAP_MINIMUM_X_Y)
    {
        mapCoord.y = MAP_MINIMUM_X_Y;
        at_map_edge = 1;
    }

    // Clamp to the map maximum value (scenario specific)
    if (mapCoord.x > gMapSizeMinus2)
    {
        mapCoord.x = gMapSizeMinus2;
        at_map_edge = 1;
    }
    if (mapCoord.y > gMapSizeMinus2)
    {
        mapCoord.y = gMapSizeMinus2;
        at_map_edge = 1;
    }

    if (at_map_edge)
    {
        auto centreLoc = centre_2d_coordinates({ mapCoord, 0 }, viewport);
        if (centreLoc)
        {
            window->savedViewPos = *centreLoc;
        }
    }

    x = window->savedViewPos.x;
    y = window->savedViewPos.y;
    if (window->flags & WF_SCROLLING_TO_LOCATION)
    {
        // Moves the viewport if focusing in on an item
        uint8_t flags = 0;
        x -= viewport->viewPos.x;
        if (x < 0)
        {
            x = -x;
            flags |= 1;
        }
        y -= viewport->viewPos.y;
        if (y < 0)
        {
            y = -y;
            flags |= 2;
        }
        x = (x + 7) / 8;
        y = (y + 7) / 8;

        // If we are at the final zoom position
        if (!x && !y)
        {
            window->flags &= ~WF_SCROLLING_TO_LOCATION;
        }
        if (flags & 1)
        {
            x = -x;
        }
        if (flags & 2)
        {
            y = -y;
        }
        x += viewport->viewPos.x;
        y += viewport->viewPos.y;
    }

    viewport_move(x, y, window, viewport);
}

void viewport_update_sprite_follow(rct_window* window)
{
    if (window->viewport_target_sprite != SPRITE_INDEX_NULL && window->viewport)
    {
        rct_sprite* sprite = get_sprite(window->viewport_target_sprite);

        int32_t height = (tile_element_height({ sprite->generic.x, sprite->generic.y })) - 16;
        int32_t underground = sprite->generic.z < height;

        viewport_set_underground_flag(underground, window, window->viewport);

        auto centreLoc = centre_2d_coordinates({ sprite->generic.x, sprite->generic.y, sprite->generic.z }, window->viewport);
        if (centreLoc)
        {
            window->savedViewPos = *centreLoc;
            viewport_move(centreLoc->x, centreLoc->y, window, window->viewport);
        }
    }
}

void viewport_update_smart_sprite_follow(rct_window* window)
{
    rct_sprite* sprite = try_get_sprite(window->viewport_smart_follow_sprite);
    if (sprite == nullptr)
    {
        window->viewport_smart_follow_sprite = SPRITE_INDEX_NULL;
        window->viewport_target_sprite = SPRITE_INDEX_NULL;
    }
    else if (sprite->generic.sprite_identifier == SPRITE_IDENTIFIER_PEEP)
    {
        Peep* peep = GET_PEEP(window->viewport_smart_follow_sprite);

        if (peep->type == PEEP_TYPE_GUEST)
            viewport_update_smart_guest_follow(window, peep);
        else if (peep->type == PEEP_TYPE_STAFF)
            viewport_update_smart_staff_follow(window, peep);
    }
    else if (sprite->generic.sprite_identifier == SPRITE_IDENTIFIER_VEHICLE)
    {
        viewport_update_smart_vehicle_follow(window);
    }
    else if (
        sprite->generic.sprite_identifier == SPRITE_IDENTIFIER_MISC
        || sprite->generic.sprite_identifier == SPRITE_IDENTIFIER_LITTER)
    {
        window->viewport_focus_sprite.sprite_id = window->viewport_smart_follow_sprite;
        window->viewport_target_sprite = window->viewport_smart_follow_sprite;
    }
    else
    {
        window->viewport_smart_follow_sprite = SPRITE_INDEX_NULL;
        window->viewport_target_sprite = SPRITE_INDEX_NULL;
    }
}

viewport_focus viewport_update_smart_guest_follow(rct_window* window, Peep* peep)
{
    viewport_focus focus{};
    focus.type = VIEWPORT_FOCUS_TYPE_SPRITE;
    focus.sprite.sprite_id = peep->sprite_index;

    if (peep->state == PEEP_STATE_PICKED)
    {
        focus.sprite.sprite_id = SPRITE_INDEX_NULL;
        window->viewport_smart_follow_sprite = SPRITE_INDEX_NULL;
        window->viewport_target_sprite = SPRITE_INDEX_NULL;
        return focus;
    }
    else
    {
        bool overallFocus = true;
        if (peep->state == PEEP_STATE_ON_RIDE || peep->state == PEEP_STATE_ENTERING_RIDE
            || (peep->state == PEEP_STATE_LEAVING_RIDE && peep->x == LOCATION_NULL))
        {
            auto ride = get_ride(peep->current_ride);
            if (ride != nullptr && (ride->lifecycle_flags & RIDE_LIFECYCLE_ON_TRACK))
            {
                auto train = GET_VEHICLE(ride->vehicles[peep->current_train]);
                if (train != nullptr)
                {
                    auto car = train->GetCar(peep->current_car);
                    if (car != nullptr)
                    {
                        focus.sprite.sprite_id = car->sprite_index;
                        overallFocus = false;
                    }
                }
            }
        }
        if (peep->x == LOCATION_NULL && overallFocus)
        {
            auto ride = get_ride(peep->current_ride);
            if (ride != nullptr)
            {
                auto xy = ride->overall_view.ToTileCentre();
                focus.type = VIEWPORT_FOCUS_TYPE_COORDINATE;
                focus.coordinate.x = xy.x;
                focus.coordinate.y = xy.y;
                focus.coordinate.z = tile_element_height(xy) + (4 * COORDS_Z_STEP);
                focus.sprite.type |= VIEWPORT_FOCUS_TYPE_COORDINATE;
            }
        }
        else
        {
            focus.sprite.type |= VIEWPORT_FOCUS_TYPE_SPRITE | VIEWPORT_FOCUS_TYPE_COORDINATE;
            focus.sprite.pad_486 &= 0xFFFF;
        }
        focus.coordinate.rotation = get_current_rotation();
    }

    window->viewport_focus_sprite = focus.sprite;
    window->viewport_target_sprite = window->viewport_focus_sprite.sprite_id;
    return focus;
}

void viewport_update_smart_staff_follow(rct_window* window, Peep* peep)
{
    sprite_focus focus = {};

    focus.sprite_id = window->viewport_smart_follow_sprite;

    if (peep->state == PEEP_STATE_PICKED)
    {
        // focus.sprite.sprite_id = SPRITE_INDEX_NULL;
        window->viewport_smart_follow_sprite = SPRITE_INDEX_NULL;
        window->viewport_target_sprite = SPRITE_INDEX_NULL;
        return;
    }
    else
    {
        focus.type |= VIEWPORT_FOCUS_TYPE_SPRITE | VIEWPORT_FOCUS_TYPE_COORDINATE;
    }

    window->viewport_focus_sprite = focus;
    window->viewport_target_sprite = window->viewport_focus_sprite.sprite_id;
}

void viewport_update_smart_vehicle_follow(rct_window* window)
{
    // Can be expanded in the future if needed
    sprite_focus focus = {};

    focus.sprite_id = window->viewport_smart_follow_sprite;

    window->viewport_focus_sprite = focus;
    window->viewport_target_sprite = window->viewport_focus_sprite.sprite_id;
}

/**
 *
 *  rct2: 0x00685C02
 *  ax: left
 *  bx: top
 *  dx: right
 *  esi: viewport
 *  edi: dpi
 *  ebp: bottom
 */
void viewport_render(
    rct_drawpixelinfo* dpi, const rct_viewport* viewport, int32_t left, int32_t top, int32_t right, int32_t bottom,
    std::vector<paint_session>* sessions)
{
    if (right <= viewport->pos.x)
        return;
    if (bottom <= viewport->pos.y)
        return;
    if (left >= viewport->pos.x + viewport->width)
        return;
    if (top >= viewport->pos.y + viewport->height)
        return;

#ifdef DEBUG_SHOW_DIRTY_BOX
    int32_t l = left, t = top, r = right, b = bottom;
#endif

    left = std::max<int32_t>(left - viewport->pos.x, 0);
    right = std::min<int32_t>(right - viewport->pos.x, viewport->width);
    top = std::max<int32_t>(top - viewport->pos.y, 0);
    bottom = std::min<int32_t>(bottom - viewport->pos.y, viewport->height);

    left = left * viewport->zoom;
    right = right * viewport->zoom;
    top = top * viewport->zoom;
    bottom = bottom * viewport->zoom;

    left += viewport->viewPos.x;
    right += viewport->viewPos.x;
    top += viewport->viewPos.y;
    bottom += viewport->viewPos.y;

    viewport_paint(viewport, dpi, left, top, right, bottom, sessions);

#ifdef DEBUG_SHOW_DIRTY_BOX
    if (viewport != g_viewport_list)
    {
        gfx_fill_rect_inset(dpi, l, t, r - 1, b - 1, 0x2, INSET_RECT_F_30);
        return;
    }
#endif
}

static void record_session(const paint_session* session, std::vector<paint_session>* recorded_sessions, size_t record_index)
{
    // Perform a deep copy of the paint session, use relative offsets.
    // This is done to extract the session for benchmark.
    // Place the copied session at provided record_index, so the caller can decide which columns/paint sessions to copy; there
    // is no column information embedded in the session itself.
    (*recorded_sessions)[record_index] = (*session);
    paint_session* session_copy = &recorded_sessions->at(record_index);

    // Mind the offset needs to be calculated against the original `session`, not `session_copy`
    for (auto& ps : session_copy->PaintStructs)
    {
        ps.basic.next_quadrant_ps = (paint_struct*)(ps.basic.next_quadrant_ps ? int(ps.basic.next_quadrant_ps - &session->PaintStructs[0].basic) : std::size(session->PaintStructs));
    }
    for (auto& quad : session_copy->Quadrants)
    {
        quad = (paint_struct*)(quad ? int(quad - &session->PaintStructs[0].basic) : std::size(session->Quadrants));
    }
}

static void viewport_fill_column(paint_session* session, std::vector<paint_session>* recorded_sessions, size_t record_index)
{
    paint_session_generate(session);
    if (recorded_sessions != nullptr)
    {
        record_session(session, recorded_sessions, record_index);
    }
    paint_session_arrange(session);
}

static void viewport_paint_column(paint_session* session)
{
    if (session->ViewFlags
            & (VIEWPORT_FLAG_HIDE_VERTICAL | VIEWPORT_FLAG_HIDE_BASE | VIEWPORT_FLAG_UNDERGROUND_INSIDE
               | VIEWPORT_FLAG_CLIP_VIEW)
        && (~session->ViewFlags & VIEWPORT_FLAG_TRANSPARENT_BACKGROUND))
    {
        uint8_t colour = COLOUR_AQUAMARINE;
        if (session->ViewFlags & VIEWPORT_FLAG_INVISIBLE_SPRITES)
        {
            colour = COLOUR_BLACK;
        }
        gfx_clear(&session->DPI, colour);
    }

    paint_draw_structs(session);

    if (gConfigGeneral.render_weather_gloom && !gTrackDesignSaveMode && !(session->ViewFlags & VIEWPORT_FLAG_INVISIBLE_SPRITES)
        && !(session->ViewFlags & VIEWPORT_FLAG_HIGHLIGHT_PATH_ISSUES))
    {
        viewport_paint_weather_gloom(&session->DPI);
    }

    if (session->PSStringHead != nullptr)
    {
        paint_draw_money_structs(&session->DPI, session->PSStringHead);
    }

    paint_session_free(session);
}

/**
 *
 *  rct2: 0x00685CBF
 *  eax: left
 *  ebx: top
 *  edx: right
 *  esi: viewport
 *  edi: dpi
 *  ebp: bottom
 */
void viewport_paint(
    const rct_viewport* viewport, rct_drawpixelinfo* dpi, int16_t left, int16_t top, int16_t right, int16_t bottom,
    std::vector<paint_session>* recorded_sessions)
{
    uint32_t viewFlags = viewport->flags;
    uint16_t width = right - left;
    uint16_t height = bottom - top;
    uint16_t bitmask = viewport->zoom >= 0 ? 0xFFFF & (0xFFFF * viewport->zoom) : 0xFFFF;

    width &= bitmask;
    height &= bitmask;
    left &= bitmask;
    top &= bitmask;
    right = left + width;
    bottom = top + height;

    int16_t x = (int16_t)(left - (int16_t)(viewport->viewPos.x & bitmask));
    x = x / viewport->zoom;
    x += viewport->pos.x;

    int16_t y = (int16_t)(top - (int16_t)(viewport->viewPos.y & bitmask));
    y = y / viewport->zoom;
    y += viewport->pos.y;

    rct_drawpixelinfo dpi1 = *dpi;
    dpi1.bits = dpi->bits + (x - dpi->x) + ((y - dpi->y) * (dpi->width + dpi->pitch));
    dpi1.x = left;
    dpi1.y = top;
    dpi1.width = width;
    dpi1.height = height;
    dpi1.pitch = (dpi->width + dpi->pitch) - (width / viewport->zoom);
    dpi1.zoom_level = viewport->zoom;

    // make sure, the compare operation is done in int16_t to avoid the loop becoming an infiniteloop.
    // this as well as the [x += 32] in the loop causes signed integer overflow -> undefined behaviour.
    const int16_t rightBorder = dpi1.x + dpi1.width;
    const int16_t alignedX = floor2(dpi1.x, 32);

    std::vector<paint_session*> columns;

    bool useMultithreading = gConfigGeneral.multithreading;
    if (useMultithreading && _paintJobs == nullptr)
    {
        _paintJobs = std::make_unique<JobPool>();
    }
    else if (useMultithreading == false && _paintJobs != nullptr)
    {
        _paintJobs.reset();
    }

    // Create space to record sessions and keep track which index is being drawn
    size_t index = 0;
    if (recorded_sessions != nullptr)
    {
        const uint16_t columnSize = rightBorder - alignedX;
        const uint16_t columnCount = (columnSize + 31) / 32;
        recorded_sessions->resize(columnCount);
    }

    // Splits the area into 32 pixel columns and renders them
    for (x = alignedX; x < rightBorder; x += 32, index++)
    {
        paint_session* session = paint_session_alloc(&dpi1, viewFlags);
        columns.push_back(session);

        rct_drawpixelinfo& dpi2 = session->DPI;
        if (x >= dpi2.x)
        {
            int16_t leftPitch = x - dpi2.x;
            dpi2.width -= leftPitch;
            dpi2.bits += leftPitch / dpi2.zoom_level;
            dpi2.pitch += leftPitch / dpi2.zoom_level;
            dpi2.x = x;
        }

        int16_t paintRight = dpi2.x + dpi2.width;
        if (paintRight >= x + 32)
        {
            int16_t rightPitch = paintRight - x - 32;
            paintRight -= rightPitch;
            dpi2.pitch += rightPitch / dpi2.zoom_level;
        }
        dpi2.width = paintRight - dpi2.x;

        if (useMultithreading)
        {
            _paintJobs->AddTask(
                [session, recorded_sessions, index]() -> void { viewport_fill_column(session, recorded_sessions, index); });
        }
        else
        {
            viewport_fill_column(session, recorded_sessions, index);
        }
    }

    if (useMultithreading)
    {
        _paintJobs->Join();
    }

    for (auto&& column : columns)
    {
        viewport_paint_column(column);
    }
}

static void viewport_paint_weather_gloom(rct_drawpixelinfo* dpi)
{
    auto paletteId = climate_get_weather_gloom_palette_id(gClimateCurrent);
    if (paletteId != PALETTE_NULL)
    {
        gfx_filter_rect(dpi, dpi->x, dpi->y, dpi->width + dpi->x - 1, dpi->height + dpi->y - 1, paletteId);
    }
}

/**
 *
 *  rct2: 0x0068958D
 */
std::optional<CoordsXY> screen_pos_to_map_pos(const ScreenCoordsXY& screenCoords, int32_t* direction)
{
    auto mapCoords = screen_get_map_xy(screenCoords, nullptr);
    if (!mapCoords)
        return std::nullopt;

    int32_t my_direction;
    int32_t dist_from_centre_x = abs(mapCoords->x % 32);
    int32_t dist_from_centre_y = abs(mapCoords->y % 32);
    if (dist_from_centre_x > 8 && dist_from_centre_x < 24 && dist_from_centre_y > 8 && dist_from_centre_y < 24)
    {
        my_direction = 4;
    }
    else
    {
        int16_t mod_x = mapCoords->x & 0x1F;
        int16_t mod_y = mapCoords->y & 0x1F;
        if (mod_x <= 16)
        {
            if (mod_y < 16)
            {
                my_direction = 2;
            }
            else
            {
                my_direction = 3;
            }
        }
        else
        {
            if (mod_y < 16)
            {
                my_direction = 1;
            }
            else
            {
                my_direction = 0;
            }
        }
    }

    if (direction != nullptr)
        *direction = my_direction;
    return { mapCoords->ToTileStart() };
}

ScreenCoordsXY screen_coord_to_viewport_coord(rct_viewport* viewport, const ScreenCoordsXY& screenCoords)
{
    ScreenCoordsXY ret;
    ret.x = ((screenCoords.x - viewport->pos.x) * viewport->zoom) + viewport->viewPos.x;
    ret.y = ((screenCoords.y - viewport->pos.y) * viewport->zoom) + viewport->viewPos.y;
    return ret;
}

CoordsXY viewport_coord_to_map_coord(int32_t x, int32_t y, int32_t z)
{
    CoordsXY ret{};
    switch (get_current_rotation())
    {
        case 0:
            ret.x = -x / 2 + y + z;
            ret.y = x / 2 + y + z;
            break;
        case 1:
            ret.x = -x / 2 - y - z;
            ret.y = -x / 2 + y + z;
            break;
        case 2:
            ret.x = x / 2 - y - z;
            ret.y = -x / 2 - y - z;
            break;
        case 3:
            ret.x = x / 2 + y + z;
            ret.y = x / 2 - y - z;
            break;
    }
    return ret;
}

/**
 *
 *  rct2: 0x00664689
 */
void show_gridlines()
{
    if (gShowGridLinesRefCount == 0)
    {
        rct_window* mainWindow = window_get_main();
        if (mainWindow != nullptr)
        {
            if (!(mainWindow->viewport->flags & VIEWPORT_FLAG_GRIDLINES))
            {
                mainWindow->viewport->flags |= VIEWPORT_FLAG_GRIDLINES;
                mainWindow->Invalidate();
            }
        }
    }
    gShowGridLinesRefCount++;
}

/**
 *
 *  rct2: 0x006646B4
 */
void hide_gridlines()
{
    gShowGridLinesRefCount--;
    if (gShowGridLinesRefCount == 0)
    {
        rct_window* mainWindow = window_get_main();
        if (mainWindow != nullptr)
        {
            if (!gConfigGeneral.always_show_gridlines)
            {
                mainWindow->viewport->flags &= ~VIEWPORT_FLAG_GRIDLINES;
                mainWindow->Invalidate();
            }
        }
    }
}

/**
 *
 *  rct2: 0x00664E8E
 */
void show_land_rights()
{
    if (gShowLandRightsRefCount == 0)
    {
        rct_window* mainWindow = window_get_main();
        if (mainWindow != nullptr)
        {
            if (!(mainWindow->viewport->flags & VIEWPORT_FLAG_LAND_OWNERSHIP))
            {
                mainWindow->viewport->flags |= VIEWPORT_FLAG_LAND_OWNERSHIP;
                mainWindow->Invalidate();
            }
        }
    }
    gShowLandRightsRefCount++;
}

/**
 *
 *  rct2: 0x00664EB9
 */
void hide_land_rights()
{
    gShowLandRightsRefCount--;
    if (gShowLandRightsRefCount == 0)
    {
        rct_window* mainWindow = window_get_main();
        if (mainWindow != nullptr)
        {
            if (mainWindow->viewport->flags & VIEWPORT_FLAG_LAND_OWNERSHIP)
            {
                mainWindow->viewport->flags &= ~VIEWPORT_FLAG_LAND_OWNERSHIP;
                mainWindow->Invalidate();
            }
        }
    }
}

/**
 *
 *  rct2: 0x00664EDD
 */
void show_construction_rights()
{
    if (gShowConstuctionRightsRefCount == 0)
    {
        rct_window* mainWindow = window_get_main();
        if (mainWindow != nullptr)
        {
            if (!(mainWindow->viewport->flags & VIEWPORT_FLAG_CONSTRUCTION_RIGHTS))
            {
                mainWindow->viewport->flags |= VIEWPORT_FLAG_CONSTRUCTION_RIGHTS;
                mainWindow->Invalidate();
            }
        }
    }
    gShowConstuctionRightsRefCount++;
}

/**
 *
 *  rct2: 0x00664F08
 */
void hide_construction_rights()
{
    gShowConstuctionRightsRefCount--;
    if (gShowConstuctionRightsRefCount == 0)
    {
        rct_window* mainWindow = window_get_main();
        if (mainWindow != nullptr)
        {
            if (mainWindow->viewport->flags & VIEWPORT_FLAG_CONSTRUCTION_RIGHTS)
            {
                mainWindow->viewport->flags &= ~VIEWPORT_FLAG_CONSTRUCTION_RIGHTS;
                mainWindow->Invalidate();
            }
        }
    }
}

/**
 *
 *  rct2: 0x006CB70A
 */
void viewport_set_visibility(uint8_t mode)
{
    rct_window* window = window_get_main();

    if (window != nullptr)
    {
        rct_viewport* vp = window->viewport;
        uint32_t invalidate = 0;

        switch (mode)
        {
            case 0:
            { // Set all these flags to 0, and invalidate if any were active
                uint32_t mask = VIEWPORT_FLAG_UNDERGROUND_INSIDE | VIEWPORT_FLAG_SEETHROUGH_RIDES
                    | VIEWPORT_FLAG_SEETHROUGH_SCENERY | VIEWPORT_FLAG_SEETHROUGH_PATHS | VIEWPORT_FLAG_INVISIBLE_SUPPORTS
                    | VIEWPORT_FLAG_LAND_HEIGHTS | VIEWPORT_FLAG_TRACK_HEIGHTS | VIEWPORT_FLAG_PATH_HEIGHTS
                    | VIEWPORT_FLAG_INVISIBLE_PEEPS | VIEWPORT_FLAG_HIDE_BASE | VIEWPORT_FLAG_HIDE_VERTICAL;

                invalidate += vp->flags & mask;
                vp->flags &= ~mask;
                break;
            }
            case 1: // 6CB79D
            case 4: // 6CB7C4
                // Set underground on, invalidate if it was off
                invalidate += !(vp->flags & VIEWPORT_FLAG_UNDERGROUND_INSIDE);
                vp->flags |= VIEWPORT_FLAG_UNDERGROUND_INSIDE;
                break;
            case 2: // 6CB7EB
                // Set track heights on, invalidate if off
                invalidate += !(vp->flags & VIEWPORT_FLAG_TRACK_HEIGHTS);
                vp->flags |= VIEWPORT_FLAG_TRACK_HEIGHTS;
                break;
            case 3: // 6CB7B1
            case 5: // 6CB7D8
                // Set underground off, invalidate if it was on
                invalidate += vp->flags & VIEWPORT_FLAG_UNDERGROUND_INSIDE;
                vp->flags &= ~((uint16_t)VIEWPORT_FLAG_UNDERGROUND_INSIDE);
                break;
        }
        if (invalidate != 0)
            window->Invalidate();
    }
}

/**
 * Checks if a paint_struct sprite type is in the filter mask.
 */
static bool PSSpriteTypeIsInFilter(paint_struct* ps, uint16_t filter)
{
    if (ps->sprite_type == VIEWPORT_INTERACTION_ITEM_NONE
        || ps->sprite_type == 11 // 11 as a type seems to not exist, maybe part of the typo mentioned later on.
        || ps->sprite_type > VIEWPORT_INTERACTION_ITEM_BANNER)
        return false;

    uint16_t mask;
    if (ps->sprite_type == VIEWPORT_INTERACTION_ITEM_BANNER)
        // I think CS made a typo here. Let's replicate the original behaviour.
        mask = 1 << (ps->sprite_type - 3);
    else
        mask = 1 << (ps->sprite_type - 1);

    if (filter & mask)
    {
        return false;
    }

    return true;
}

/**
 * rct2: 0x00679236, 0x00679662, 0x00679B0D, 0x00679FF1
 */
static bool is_pixel_present_bmp(uint32_t imageType, const rct_g1_element* g1, const uint8_t* index, const uint8_t* palette)
{
    // Probably used to check for corruption
    if (!(g1->flags & G1_FLAG_BMP))
    {
        return false;
    }

    if (imageType & IMAGE_TYPE_REMAP)
    {
        return palette[*index] != 0;
    }

    if (imageType & IMAGE_TYPE_TRANSPARENT)
    {
        return false;
    }

    return (*index != 0);
}

/**
 * rct2: 0x0067933B, 0x00679788, 0x00679C4A, 0x0067A117
 */
static bool is_pixel_present_rle(const uint8_t* esi, int16_t x_start_point, int16_t y_start_point, int32_t round)
{
    uint16_t start_offset = esi[y_start_point * 2] | (esi[y_start_point * 2 + 1] << 8);
    const uint8_t* ebx = esi + start_offset;

    uint8_t last_data_line = 0;
    while (!last_data_line)
    {
        int32_t no_pixels = *ebx++;
        uint8_t gap_size = *ebx++;

        last_data_line = no_pixels & 0x80;

        no_pixels &= 0x7F;

        ebx += no_pixels;

        if (round > 1)
        {
            if (gap_size % 2)
            {
                gap_size++;
                no_pixels--;
                if (no_pixels == 0)
                {
                    continue;
                }
            }
        }

        if (round == 4)
        {
            if (gap_size % 4)
            {
                gap_size += 2;
                no_pixels -= 2;
                if (no_pixels <= 0)
                {
                    continue;
                }
            }
        }

        int32_t x_start = gap_size - x_start_point;
        if (x_start <= 0)
        {
            no_pixels += x_start;
            if (no_pixels <= 0)
            {
                continue;
            }

            x_start = 0;
        }
        else
        {
            // Do nothing?
        }

        x_start += no_pixels;
        x_start--;
        if (x_start > 0)
        {
            no_pixels -= x_start;
            if (no_pixels <= 0)
            {
                continue;
            }
        }

        if (round > 1)
        {
            // This matches the original implementation, but allows empty lines to cause false positives on zoom 0
            if (ceil2(no_pixels, round) == 0)
                continue;
        }

        return true;
    }

    return false;
}

/**
 * rct2: 0x00679074
 *
 * @param dpi (edi)
 * @param imageId (ebx)
 * @param x (cx)
 * @param y (dx)
 * @return value originally stored in 0x00141F569
 */
static bool is_sprite_interacted_with_palette_set(
    rct_drawpixelinfo* dpi, int32_t imageId, int16_t x, int16_t y, const uint8_t* palette)
{
    const rct_g1_element* g1 = gfx_get_g1_element(imageId & 0x7FFFF);
    if (g1 == nullptr)
    {
        return false;
    }

    if (dpi->zoom_level > 0)
    {
        if (g1->flags & G1_FLAG_NO_ZOOM_DRAW)
        {
            return false;
        }

        if (g1->flags & G1_FLAG_HAS_ZOOM_SPRITE)
        {
            // TODO: SAR in dpi done with `>> 1`, in coordinates with `/ 2`
            rct_drawpixelinfo zoomed_dpi = {
                /* .bits = */ dpi->bits,
                /* .x = */ (int16_t)(dpi->x >> 1),
                /* .y = */ (int16_t)(dpi->y >> 1),
                /* .height = */ dpi->height,
                /* .width = */ dpi->width,
                /* .pitch = */ dpi->pitch,
                /* .zoom_level = */ dpi->zoom_level - 1,
            };

            return is_sprite_interacted_with_palette_set(&zoomed_dpi, imageId - g1->zoomed_offset, x / 2, y / 2, palette);
        }
    }

    int32_t round = std::max(1, 1 * dpi->zoom_level);

    if (g1->flags & G1_FLAG_RLE_COMPRESSION)
    {
        y -= (round - 1);
    }

    y += g1->y_offset;
    int16_t yStartPoint = 0;
    int16_t height = g1->height;
    if (dpi->zoom_level != 0)
    {
        if (height % 2)
        {
            height--;
            yStartPoint++;
        }

        if (dpi->zoom_level == 2)
        {
            if (height % 4)
            {
                height -= 2;
                yStartPoint += 2;
            }
        }

        if (height == 0)
        {
            return false;
        }
    }

    y = floor2(y, round);
    int16_t yEndPoint = height;
    y -= dpi->y;
    if (y < 0)
    {
        yEndPoint += y;
        if (yEndPoint <= 0)
        {
            return false;
        }

        yStartPoint -= y;
        y = 0;
    }

    y += yEndPoint;
    y--;
    if (y > 0)
    {
        yEndPoint -= y;
        if (yEndPoint <= 0)
        {
            return false;
        }
    }

    int16_t xStartPoint = 0;
    int16_t xEndPoint = g1->width;

    x += g1->x_offset;
    x = floor2(x, round);
    x -= dpi->x;
    if (x < 0)
    {
        xEndPoint += x;
        if (xEndPoint <= 0)
        {
            return false;
        }

        xStartPoint -= x;
        x = 0;
    }

    x += xEndPoint;
    x--;
    if (x > 0)
    {
        xEndPoint -= x;
        if (xEndPoint <= 0)
        {
            return false;
        }
    }

    if (g1->flags & G1_FLAG_RLE_COMPRESSION)
    {
        return is_pixel_present_rle(g1->offset, xStartPoint, yStartPoint, round);
    }

    uint8_t* offset = g1->offset + (yStartPoint * g1->width) + xStartPoint;
    uint32_t imageType = _currentImageType;

    if (!(g1->flags & G1_FLAG_1))
    {
        return is_pixel_present_bmp(imageType, g1, offset, palette);
    }

    Guard::Assert(false, "Invalid image type encountered.");
    return false;
}

/**
 *
 *  rct2: 0x00679023
 */
static bool is_sprite_interacted_with(rct_drawpixelinfo* dpi, int32_t imageId, int32_t x, int32_t y)
{
    const uint8_t* palette = nullptr;
    imageId &= ~IMAGE_TYPE_TRANSPARENT;
    if (imageId & IMAGE_TYPE_REMAP)
    {
        _currentImageType = IMAGE_TYPE_REMAP;
        int32_t index = (imageId >> 19) & 0x7F;
        if (imageId & IMAGE_TYPE_REMAP_2_PLUS)
        {
            index &= 0x1F;
        }
        int32_t g1Index = palette_to_g1_offset[index];
        const rct_g1_element* g1 = gfx_get_g1_element(g1Index);
        if (g1 != nullptr)
        {
            palette = g1->offset;
        }
    }
    else
    {
        _currentImageType = 0;
    }
    return is_sprite_interacted_with_palette_set(dpi, imageId, x, y, palette);
}

/**
 *
 *  rct2: 0x0068862C
 */
InteractionInfo set_interaction_info_from_paint_session(paint_session* session, uint16_t filter)
{
    paint_struct* ps = &session->PaintHead;
    rct_drawpixelinfo* dpi = &session->DPI;
    InteractionInfo info{};

    while ((ps = ps->next_quadrant_ps) != nullptr)
    {
        paint_struct* old_ps = ps;
        paint_struct* next_ps = ps;
        while (next_ps != nullptr)
        {
            ps = next_ps;
            if (is_sprite_interacted_with(dpi, ps->image_id, ps->x, ps->y))
            {
                if (PSSpriteTypeIsInFilter(ps, filter))
                {
                    info = { ps };
                }
            }
            next_ps = ps->children;
        }

        for (attached_paint_struct* attached_ps = ps->attached_ps; attached_ps != nullptr; attached_ps = attached_ps->next)
        {
            if (is_sprite_interacted_with(
                    dpi, attached_ps->image_id, (attached_ps->x + ps->x) & 0xFFFF, (attached_ps->y + ps->y) & 0xFFFF))
            {
                if (PSSpriteTypeIsInFilter(ps, filter))
                {
                    info = { ps };
                }
            }
        }

        ps = old_ps;
    }
    return info;
}

/**
 *
 *  rct2: 0x00685ADC
 * screenX: eax
 * screenY: ebx
 * flags: edx
 * x: ax
 * y: cx
 * interactionType: bl
 * tileElement: edx
 * viewport: edi
 */
void get_map_coordinates_from_pos(
    const ScreenCoordsXY& screenCoords, int32_t flags, CoordsXY& mapCoords, int32_t* interactionType, TileElement** tileElement,
    rct_viewport** viewport)
{
    rct_window* window = window_find_from_point(screenCoords);
    get_map_coordinates_from_pos_window(window, screenCoords, flags, mapCoords, interactionType, tileElement, viewport);
}

void get_map_coordinates_from_pos_window(
    rct_window* window, ScreenCoordsXY screenCoords, int32_t flags, CoordsXY& mapCoords, int32_t* interactionType,
    TileElement** tileElement, rct_viewport** viewport)
{
    InteractionInfo info{};
    if (window != nullptr && window->viewport != nullptr)
    {
        rct_viewport* myviewport = window->viewport;
        screenCoords -= myviewport->pos;
        if (screenCoords.x >= 0 && screenCoords.x < (int32_t)myviewport->width && screenCoords.y >= 0
            && screenCoords.y < (int32_t)myviewport->height)
        {
            screenCoords.x = screenCoords.x * myviewport->zoom;
            screenCoords.y = screenCoords.y * myviewport->zoom;
            screenCoords += myviewport->viewPos;
            if (myviewport->zoom > 0)
            {
                screenCoords.x &= (0xFFFF * myviewport->zoom) & 0xFFFF;
                screenCoords.y &= (0xFFFF * myviewport->zoom) & 0xFFFF;
            }
            rct_drawpixelinfo dpi;
            dpi.x = screenCoords.x;
            dpi.y = screenCoords.y;
            dpi.height = 1;
            dpi.zoom_level = myviewport->zoom;
            dpi.width = 1;

            paint_session* session = paint_session_alloc(&dpi, myviewport->flags);
            paint_session_generate(session);
            paint_session_arrange(session);
            info = set_interaction_info_from_paint_session(session, flags & 0xFFFF);
            paint_session_free(session);
        }
        if (viewport != nullptr)
            *viewport = myviewport;
    }
    if (interactionType != nullptr)
        *interactionType = info.SpriteType;

    mapCoords = info.Loc;

    if (tileElement != nullptr)
        *tileElement = info.Element;
}

/**
 * Left, top, right and bottom represent 2D map coordinates at zoom 0.
 */
void viewport_invalidate(rct_viewport* viewport, int32_t left, int32_t top, int32_t right, int32_t bottom)
{
    // if unknown viewport visibility, use the containing window to discover the status
    if (viewport->visibility == VC_UNKNOWN)
    {
        auto windowManager = GetContext()->GetUiContext()->GetWindowManager();
        auto owner = windowManager->GetOwner(viewport);
        if (owner != nullptr && owner->classification != WC_MAIN_WINDOW)
        {
            // note, window_is_visible will update viewport->visibility, so this should have a low hit count
            if (!window_is_visible(owner))
            {
                return;
            }
        }
    }

    if (viewport->visibility == VC_COVERED)
        return;

    int32_t viewportLeft = viewport->viewPos.x;
    int32_t viewportTop = viewport->viewPos.y;
    int32_t viewportRight = viewport->viewPos.x + viewport->view_width;
    int32_t viewportBottom = viewport->viewPos.y + viewport->view_height;
    if (right > viewportLeft && bottom > viewportTop)
    {
        left = std::max(left, viewportLeft);
        top = std::max(top, viewportTop);
        right = std::min(right, viewportRight);
        bottom = std::min(bottom, viewportBottom);

        left -= viewportLeft;
        top -= viewportTop;
        right -= viewportLeft;
        bottom -= viewportTop;
        left = left / viewport->zoom;
        top = top / viewport->zoom;
        right = right / viewport->zoom;
        bottom = bottom / viewport->zoom;
        left += viewport->pos.x;
        top += viewport->pos.y;
        right += viewport->pos.x;
        bottom += viewport->pos.y;
        gfx_set_dirty_blocks(left, top, right, bottom);
    }
}

static rct_viewport* viewport_find_from_point(const ScreenCoordsXY& screenCoords)
{
    rct_window* w = window_find_from_point(screenCoords);
    if (w == nullptr)
        return nullptr;

    rct_viewport* viewport = w->viewport;
    if (viewport == nullptr)
        return nullptr;

    if (screenCoords.x < viewport->pos.x || screenCoords.y < viewport->pos.y)
        return nullptr;
    if (screenCoords.x >= viewport->pos.x + viewport->width || screenCoords.y >= viewport->pos.y + viewport->height)
        return nullptr;

    return viewport;
}

/**
 *
 *  rct2: 0x00688972
 * In:
 *      screen_x: eax
 *      screen_y: ebx
 * Out:
 *      x: ax
 *      y: bx
 *      tile_element: edx ?
 *      viewport: edi
 */
std::optional<CoordsXY> screen_get_map_xy(const ScreenCoordsXY& screenCoords, rct_viewport** viewport)
{
    int32_t interactionType;
    rct_viewport* myViewport = nullptr;
    CoordsXY tileLoc;
    // This will get the tile location but we will need the more accuracy
    get_map_coordinates_from_pos(
        screenCoords, VIEWPORT_INTERACTION_MASK_TERRAIN, tileLoc, &interactionType, nullptr, &myViewport);
    if (interactionType == VIEWPORT_INTERACTION_ITEM_NONE)
    {
        return std::nullopt;
    }

    auto start_vp_pos = screen_coord_to_viewport_coord(myViewport, screenCoords);
    CoordsXY cursorMapPos = { tileLoc.x + 16, tileLoc.y + 16 };

    // Iterates the cursor location to work out exactly where on the tile it is
    for (int32_t i = 0; i < 5; i++)
    {
        int32_t z = tile_element_height(cursorMapPos);
        cursorMapPos = viewport_coord_to_map_coord(start_vp_pos.x, start_vp_pos.y, z);
        cursorMapPos.x = std::clamp(cursorMapPos.x, tileLoc.x, tileLoc.x + 31);
        cursorMapPos.y = std::clamp(cursorMapPos.y, tileLoc.y, tileLoc.y + 31);
    }

    if (viewport != nullptr)
        *viewport = myViewport;

    return cursorMapPos;
}

/**
 *
 *  rct2: 0x006894D4
 */
std::optional<CoordsXY> screen_get_map_xy_with_z(const ScreenCoordsXY& screenCoords, int16_t z)
{
    rct_viewport* viewport = viewport_find_from_point(screenCoords);
    if (viewport == nullptr)
    {
        return std::nullopt;
    }

    auto vpCoords = screen_coord_to_viewport_coord(viewport, screenCoords);
    auto mapPosition = viewport_coord_to_map_coord(vpCoords.x, vpCoords.y, z);
    if (!map_is_location_valid(mapPosition))
    {
        return std::nullopt;
    }

    return mapPosition;
}

/**
 *
 *  rct2: 0x00689604
 */
std::optional<CoordsXY> screen_get_map_xy_quadrant(const ScreenCoordsXY& screenCoords, uint8_t* quadrant)
{
    auto mapCoords = screen_get_map_xy(screenCoords, nullptr);
    if (!mapCoords)
        return std::nullopt;

    *quadrant = map_get_tile_quadrant(*mapCoords);
    return mapCoords->ToTileStart();
}

/**
 *
 *  rct2: 0x0068964B
 */
std::optional<CoordsXY> screen_get_map_xy_quadrant_with_z(const ScreenCoordsXY& screenCoords, int16_t z, uint8_t* quadrant)
{
    auto mapCoords = screen_get_map_xy_with_z(screenCoords, z);
    if (!mapCoords)
        return std::nullopt;

    *quadrant = map_get_tile_quadrant(*mapCoords);
    return mapCoords->ToTileStart();
}

/**
 *
 *  rct2: 0x00689692
 */
std::optional<CoordsXY> screen_get_map_xy_side(const ScreenCoordsXY& screenCoords, uint8_t* side)
{
    auto mapCoords = screen_get_map_xy(screenCoords, nullptr);
    if (!mapCoords)
        return std::nullopt;

    *side = map_get_tile_side(*mapCoords);
    return mapCoords->ToTileStart();
}

/**
 *
 *  rct2: 0x006896DC
 */
std::optional<CoordsXY> screen_get_map_xy_side_with_z(const ScreenCoordsXY& screenCoords, int16_t z, uint8_t* side)
{
    auto mapCoords = screen_get_map_xy_with_z(screenCoords, z);
    if (!mapCoords)
        return std::nullopt;

    *side = map_get_tile_side(*mapCoords);
    return mapCoords->ToTileStart();
}

/**
 * Get current viewport rotation.
 *
 * If an invalid rotation is detected and DEBUG_LEVEL_1 is enabled, an error
 * will be reported.
 *
 * @returns rotation in range 0-3 (inclusive)
 */
uint8_t get_current_rotation()
{
    uint8_t rotation = gCurrentRotation;
    uint8_t rotation_masked = rotation & 3;
#if defined(DEBUG_LEVEL_1) && DEBUG_LEVEL_1
    if (rotation != rotation_masked)
    {
        log_error("Found wrong rotation %d! Will return %d instead.", (uint32_t)rotation, (uint32_t)rotation_masked);
    }
#endif // DEBUG_LEVEL_1
    return rotation_masked;
}

int16_t get_height_marker_offset()
{
    // Height labels in units
    if (gConfigGeneral.show_height_as_units)
        return 0;

    // Height labels in feet
    if (gConfigGeneral.measurement_format == MEASUREMENT_FORMAT_IMPERIAL)
        return 1 * 256;

    // Height labels in metres
    return 2 * 256;
}

void viewport_set_saved_view()
{
    rct_window* w = window_get_main();
    if (w != nullptr)
    {
        rct_viewport* viewport = w->viewport;

        gSavedView = ScreenCoordsXY{ viewport->view_width / 2, viewport->view_height / 2 } + viewport->viewPos;

        gSavedViewZoom = viewport->zoom;
        gSavedViewRotation = get_current_rotation();
    }
}
