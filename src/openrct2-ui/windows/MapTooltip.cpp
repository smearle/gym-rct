/*****************************************************************************
 * Copyright (c) 2014-2019 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "../interface/Theme.h"

#include <openrct2-ui/interface/Widget.h>
#include <openrct2-ui/windows/Window.h>
#include <openrct2/Context.h>
#include <openrct2/Input.h>
#include <openrct2/drawing/Drawing.h>
#include <openrct2/localisation/Localisation.h>

// clang-format off
static rct_widget window_map_tooltip_widgets[] = {
    { WWT_IMGBTN, 0, 0, 199, 0, 29, 0xFFFFFFFF, STR_NONE },
    { WIDGETS_END }
};

static void window_map_tooltip_update(rct_window *w);
static void window_map_tooltip_paint(rct_window *w, rct_drawpixelinfo *dpi);

static rct_window_event_list window_map_tooltip_events = {
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    window_map_tooltip_update,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    window_map_tooltip_paint,
    nullptr
};
// clang-format on

#define MAP_TOOLTIP_ARGS

static ScreenCoordsXY _lastCursor;
static int32_t _cursorHoldDuration;

static void window_map_tooltip_open();

/**
 *
 *  rct2: 0x006EE77A
 */
void window_map_tooltip_update_visibility()
{
    if (theme_get_flags() & UITHEME_FLAG_USE_FULL_BOTTOM_TOOLBAR)
    {
        // The map tooltip is drawn by the bottom toolbar
        window_invalidate_by_class(WC_BOTTOM_TOOLBAR);
        return;
    }

    const CursorState* state = context_get_cursor_state();
    auto cursor = state->position;
    auto cursorChange = cursor - _lastCursor;

    // Check for cursor movement
    _cursorHoldDuration++;
    if (abs(cursorChange.x) > 5 || abs(cursorChange.y) > 5 || (input_test_flag(INPUT_FLAG_5)))
        _cursorHoldDuration = 0;

    _lastCursor = cursor;

    // Show or hide tooltip
    rct_string_id stringId;
    std::memcpy(&stringId, gMapTooltipFormatArgs, sizeof(rct_string_id));

    if (_cursorHoldDuration < 25 || stringId == STR_NONE
        || input_test_place_object_modifier(
            (PLACE_OBJECT_MODIFIER)(PLACE_OBJECT_MODIFIER_COPY_Z | PLACE_OBJECT_MODIFIER_SHIFT_Z))
        || window_find_by_class(WC_ERROR) != nullptr)
    {
        window_close_by_class(WC_MAP_TOOLTIP);
    }
    else
    {
        window_map_tooltip_open();
    }
}

/**
 *
 *  rct2: 0x006A7C43
 */
static void window_map_tooltip_open()
{
    rct_window* w;

    constexpr int32_t width = 200;
    constexpr int32_t height = 44;
    const CursorState* state = context_get_cursor_state();
    ScreenCoordsXY pos = { state->position.x - (width / 2), state->position.y + 15 };

    w = window_find_by_class(WC_MAP_TOOLTIP);
    if (w == nullptr)
    {
        w = window_create(
            pos, width, height, &window_map_tooltip_events, WC_MAP_TOOLTIP,
            WF_STICK_TO_FRONT | WF_TRANSPARENT | WF_NO_BACKGROUND);
        w->widgets = window_map_tooltip_widgets;
    }
    else
    {
        w->Invalidate();
        w->windowPos = pos;
        w->width = width;
        w->height = height;
    }
}

/**
 *
 *  rct2: 0x006EE8CE
 */
static void window_map_tooltip_update(rct_window* w)
{
    w->Invalidate();
}

/**
 *
 *  rct2: 0x006EE894
 */
static void window_map_tooltip_paint(rct_window* w, rct_drawpixelinfo* dpi)
{
    rct_string_id stringId;
    std::memcpy(&stringId, gMapTooltipFormatArgs, sizeof(rct_string_id));
    if (stringId == STR_NONE)
    {
        return;
    }

    gfx_draw_string_centred_wrapped(
        dpi, gMapTooltipFormatArgs, w->windowPos.x + (w->width / 2), w->windowPos.y + (w->height / 2), w->width,
        STR_MAP_TOOLTIP_STRINGID, COLOUR_BLACK);
}
