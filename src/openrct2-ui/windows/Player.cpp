/*****************************************************************************
 * Copyright (c) 2014-2019 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include <openrct2-ui/interface/Dropdown.h>
#include <openrct2-ui/interface/Viewport.h>
#include <openrct2-ui/interface/Widget.h>
#include <openrct2-ui/windows/Window.h>
#include <openrct2/Game.h>
#include <openrct2/Input.h>
#include <openrct2/actions/PlayerKickAction.hpp>
#include <openrct2/actions/PlayerSetGroupAction.hpp>
#include <openrct2/config/Config.h>
#include <openrct2/drawing/Drawing.h>
#include <openrct2/interface/Colour.h>
#include <openrct2/localisation/Localisation.h>
#include <openrct2/network/NetworkAction.h>
#include <openrct2/network/network.h>
#include <openrct2/sprites.h>
#include <openrct2/util/Util.h>

// clang-format off
enum WINDOW_PLAYER_PAGE {
    WINDOW_PLAYER_PAGE_OVERVIEW,
    WINDOW_PLAYER_PAGE_STATISTICS,
};

#pragma region Widgets

enum WINDOW_PLAYER_WIDGET_IDX {
    WIDX_BACKGROUND,
    WIDX_TITLE,
    WIDX_CLOSE,
    WIDX_PAGE_BACKGROUND,
    WIDX_TAB_1,
    WIDX_TAB_2,

    WIDX_GROUP = 6,
    WIDX_GROUP_DROPDOWN,
    WIDX_LOCATE,
    WIDX_KICK,
    WIDX_VIEWPORT,
};

#define WINDOW_PLAYER_COMMON_WIDGETS                                                                                                    \
    { WWT_FRAME,            0,  0,      191,    0,      156,    0xFFFFFFFF,             STR_NONE },             /* Panel / Background   */      \
    { WWT_CAPTION,          0,  1,      190,    1,      14,     STR_STRING,             STR_WINDOW_TITLE_TIP }, /* Title                */      \
    { WWT_CLOSEBOX,         0,  179,    189,    2,      13,     STR_CLOSE_X,            STR_CLOSE_WINDOW_TIP }, /* Close x button       */      \
    { WWT_RESIZE,           1,  0,      191,    43,     156,    0xFFFFFFFF,             STR_NONE },             /* Resize               */      \
    { WWT_TAB,              1,  3,      33,     17,     43,     IMAGE_TYPE_REMAP | SPR_TAB,   STR_NONE },             /* Tab 1                */      \
    { WWT_TAB,              1,  34,     64,     17,     43,     IMAGE_TYPE_REMAP | SPR_TAB,   STR_NONE }              /* Tab 2                */

static rct_widget window_player_overview_widgets[] = {
    WINDOW_PLAYER_COMMON_WIDGETS,
    { WWT_DROPDOWN,         1,  3,      177,    46,     57,     0xFFFFFFFF,         STR_NONE },                 // Permission group
    { WWT_BUTTON,           1,  167,    177,    47,     56,     STR_DROPDOWN_GLYPH, STR_NONE },                 //
    { WWT_FLATBTN,          1,  179,    190,    45,     68,     SPR_LOCATE,         STR_LOCATE_PLAYER_TIP },    // Locate button
    { WWT_FLATBTN,          1,  179,    190,    69,     92,     SPR_DEMOLISH,       STR_KICK_PLAYER_TIP },      // Kick button
    { WWT_VIEWPORT,         1,  3,      177,    60,     120,    0xFFFFFFFF,         STR_NONE },                 // Viewport
    { WIDGETS_END },
};

static rct_widget window_player_statistics_widgets[] = {
    WINDOW_PLAYER_COMMON_WIDGETS,
    { WIDGETS_END },
};

static rct_widget *window_player_page_widgets[] = {
    window_player_overview_widgets,
    window_player_statistics_widgets
};

#pragma endregion

#pragma region Events

static void window_player_overview_close(rct_window *w);
static void window_player_overview_mouse_up(rct_window *w, rct_widgetindex widgetIndex);
static void window_player_overview_resize(rct_window *w);
static void window_player_overview_mouse_down(rct_window *w, rct_widgetindex widgetIndex, rct_widget *widget);
static void window_player_overview_dropdown(rct_window *w, rct_widgetindex widgetIndex, int32_t dropdownIndex);
static void window_player_overview_update(rct_window* w);
static void window_player_overview_invalidate(rct_window *w);
static void window_player_overview_paint(rct_window *w, rct_drawpixelinfo *dpi);

static rct_window_event_list window_player_overview_events = {
    window_player_overview_close,
    window_player_overview_mouse_up,
    window_player_overview_resize,
    window_player_overview_mouse_down,
    window_player_overview_dropdown,
    nullptr,
    window_player_overview_update,
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
    window_player_overview_invalidate,
    window_player_overview_paint,
    nullptr
};

static void window_player_statistics_close(rct_window *w);
static void window_player_statistics_mouse_up(rct_window *w, rct_widgetindex widgetIndex);
static void window_player_statistics_resize(rct_window *w);
static void window_player_statistics_update(rct_window* w);
static void window_player_statistics_invalidate(rct_window *w);
static void window_player_statistics_paint(rct_window *w, rct_drawpixelinfo *dpi);

static rct_window_event_list window_player_statistics_events = {
    window_player_statistics_close,
    window_player_statistics_mouse_up,
    window_player_statistics_resize,
    nullptr,
    nullptr,
    nullptr,
    window_player_statistics_update,
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
    window_player_statistics_invalidate,
    window_player_statistics_paint,
    nullptr
};

static rct_window_event_list *window_player_page_events[] = {
    &window_player_overview_events,
    &window_player_statistics_events
};

#pragma endregion

static void window_player_set_page(rct_window* w, int32_t page);
static void window_player_draw_tab_images(rct_drawpixelinfo *dpi, rct_window *w);
static void window_player_update_viewport(rct_window *w, bool scroll);
static void window_player_update_title(rct_window* w);

static uint32_t window_player_page_enabled_widgets[] = {
    (1 << WIDX_CLOSE) |
    (1 << WIDX_TAB_1) |
    (1 << WIDX_TAB_2) |
    (1 << WIDX_GROUP) |
    (1 << WIDX_GROUP_DROPDOWN) |
    (1 << WIDX_LOCATE) |
    (1 << WIDX_KICK),

    (1 << WIDX_CLOSE) |
    (1 << WIDX_TAB_1) |
    (1 << WIDX_TAB_2)
};
// clang-format on

rct_window* window_player_open(uint8_t id)
{
    rct_window* window;

    window = window_bring_to_front_by_number(WC_PLAYER, id);
    if (window == nullptr)
    {
        window = window_create_auto_pos(240, 170, &window_player_overview_events, WC_PLAYER, WF_RESIZABLE);
        window->number = id;
        window->page = 0;
        window->viewport_focus_coordinates.y = 0;
        window->frame_no = 0;
        window->list_information_type = 0;
        window->picked_peep_frame = 0;
        window->highlighted_item = 0;
        window->min_width = 210;
        window->min_height = 134;
        window->max_width = 500;
        window->max_height = 450;
        window->no_list_items = 0;
        window->selected_list_item = -1;

        window->viewport_focus_coordinates.y = -1;
    }

    window->page = 0;
    window->Invalidate();

    window->widgets = window_player_page_widgets[WINDOW_PLAYER_PAGE_OVERVIEW];
    window->enabled_widgets = window_player_page_enabled_widgets[WINDOW_PLAYER_PAGE_OVERVIEW];
    window->hold_down_widgets = 0;
    window->event_handlers = window_player_page_events[WINDOW_PLAYER_PAGE_OVERVIEW];
    window->pressed_widgets = 0;

    window_init_scroll_widgets(window);
    window_player_set_page(window, WINDOW_PLAYER_PAGE_OVERVIEW);

    return window;
}

static void window_player_overview_show_group_dropdown(rct_window* w, rct_widget* widget)
{
    rct_widget* dropdownWidget;
    int32_t numItems, i;
    int32_t player = network_get_player_index((uint8_t)w->number);
    if (player == -1)
    {
        return;
    }

    dropdownWidget = widget - 1;

    numItems = network_get_num_groups();

    window_dropdown_show_text_custom_width(
        w->windowPos.x + dropdownWidget->left, w->windowPos.y + dropdownWidget->top,
        dropdownWidget->bottom - dropdownWidget->top + 1, w->colours[1], 0, 0, numItems, widget->right - dropdownWidget->left);

    for (i = 0; i < network_get_num_groups(); i++)
    {
        gDropdownItemsFormat[i] = STR_OPTIONS_DROPDOWN_ITEM;
        gDropdownItemsArgs[i] = (uintptr_t)network_get_group_name(i);
    }

    dropdown_set_checked(network_get_group_index(network_get_player_group(player)), true);
}

void window_player_overview_close(rct_window* w)
{
}

void window_player_overview_mouse_up(rct_window* w, rct_widgetindex widgetIndex)
{
    switch (widgetIndex)
    {
        case WIDX_CLOSE:
            window_close(w);
            break;
        case WIDX_TAB_1:
        case WIDX_TAB_2:
            window_player_set_page(w, widgetIndex - WIDX_TAB_1);
            break;
        case WIDX_LOCATE:
        {
            rct_window* mainWindow = window_get_main();
            if (mainWindow != nullptr)
            {
                int32_t player = network_get_player_index((uint8_t)w->number);
                if (player == -1)
                {
                    return;
                }
                auto coord = network_get_player_last_action_coord(player);
                if (coord.x || coord.y || coord.z)
                {
                    window_scroll_to_location(mainWindow, coord.x, coord.y, coord.z);
                }
            }
        }
        break;
        case WIDX_KICK:
        {
            auto kickPlayerAction = PlayerKickAction(w->number);
            GameActions::Execute(&kickPlayerAction);
        }
        break;
    }
}

void window_player_overview_mouse_down(rct_window* w, rct_widgetindex widgetIndex, rct_widget* widget)
{
    switch (widgetIndex)
    {
        case WIDX_GROUP_DROPDOWN:
            window_player_overview_show_group_dropdown(w, widget);
            break;
    }
}

void window_player_overview_dropdown(rct_window* w, rct_widgetindex widgetIndex, int32_t dropdownIndex)
{
    int32_t player = network_get_player_index((uint8_t)w->number);
    if (player == -1)
    {
        return;
    }
    if (dropdownIndex == -1)
    {
        return;
    }
    int32_t group = network_get_group_id(dropdownIndex);
    auto playerSetGroupAction = PlayerSetGroupAction(w->number, group);
    playerSetGroupAction.SetCallback([=](const GameAction* ga, const GameActionResult* result) {
        if (result->Error == GA_ERROR::OK)
        {
            w->Invalidate();
        }
    });
    GameActions::Execute(&playerSetGroupAction);
}

void window_player_overview_resize(rct_window* w)
{
    window_set_resize(w, 240, 170, 500, 300);
}

void window_player_overview_update(rct_window* w)
{
    w->frame_no++;
    widget_invalidate(w, WIDX_TAB_1 + w->page);

    if (network_get_player_index((uint8_t)w->number) == -1)
    {
        window_close(w);
        return;
    }

    // Update viewport
    bool scroll = true;

    // Use this spare window field for rotation check
    if (w->var_4AE != get_current_rotation())
    {
        w->var_4AE = get_current_rotation();
        scroll = false;
    }
    window_player_update_viewport(w, scroll);
}

void window_player_overview_paint(rct_window* w, rct_drawpixelinfo* dpi)
{
    window_draw_widgets(w, dpi);
    window_player_draw_tab_images(dpi, w);

    int32_t player = network_get_player_index((uint8_t)w->number);
    if (player == -1)
    {
        return;
    }

    // Draw current group
    int32_t groupindex = network_get_group_index(network_get_player_group(player));
    if (groupindex != -1)
    {
        rct_widget* widget = &window_player_overview_widgets[WIDX_GROUP];
        char buffer[300];
        char* lineCh;
        lineCh = buffer;
        lineCh = utf8_write_codepoint(lineCh, FORMAT_WINDOW_COLOUR_2);
        safe_strcpy(lineCh, network_get_group_name(groupindex), sizeof(buffer) - (lineCh - buffer));
        set_format_arg(0, const char*, buffer);

        gfx_draw_string_centred_clipped(
            dpi, STR_STRING, gCommonFormatArgs, COLOUR_BLACK, w->windowPos.x + (widget->left + widget->right - 11) / 2,
            w->windowPos.y + widget->top, widget->right - widget->left - 8);
    }

    // Draw ping
    int32_t x = w->windowPos.x + 90;
    int32_t y = w->windowPos.y + 24;

    set_format_arg(0, rct_string_id, STR_PING);
    gfx_draw_string_left(dpi, STR_WINDOW_COLOUR_2_STRINGID, gCommonFormatArgs, 0, x, y);
    char ping[64];
    snprintf(ping, 64, "%d ms", network_get_player_ping(player));
    gfx_draw_string(dpi, ping, w->colours[2], x + 30, y);

    // Draw last action
    x = w->windowPos.x + (w->width / 2);
    y = w->windowPos.y + w->height - 13;
    int32_t width = w->width - 8;
    int32_t lastaction = network_get_player_last_action(player, 0);
    set_format_arg(0, rct_string_id, STR_ACTION_NA);
    if (lastaction != -999)
    {
        set_format_arg(0, rct_string_id, network_get_action_name_string_id(lastaction));
    }
    gfx_draw_string_centred_clipped(dpi, STR_LAST_ACTION_RAN, gCommonFormatArgs, COLOUR_BLACK, x, y, width);

    if (w->viewport != nullptr && w->var_492 != -1)
    {
        window_draw_viewport(dpi, w);
    }
}

void window_player_overview_invalidate(rct_window* w)
{
    int32_t playerIndex = network_get_player_index((uint8_t)w->number);
    if (playerIndex == -1)
    {
        return;
    }

    if (window_player_page_widgets[w->page] != w->widgets)
    {
        w->widgets = window_player_page_widgets[w->page];
        window_init_scroll_widgets(w);
    }

    w->pressed_widgets &= ~(WIDX_TAB_1);
    w->pressed_widgets &= ~(WIDX_TAB_2);
    w->pressed_widgets |= 1ULL << (w->page + WIDX_TAB_1);

    window_player_update_title(w);

    w->widgets[WIDX_BACKGROUND].right = w->width - 1;
    w->widgets[WIDX_BACKGROUND].bottom = w->height - 1;
    w->widgets[WIDX_PAGE_BACKGROUND].right = w->width - 1;
    w->widgets[WIDX_PAGE_BACKGROUND].bottom = w->height - 1;
    w->widgets[WIDX_TITLE].right = w->width - 2;
    w->widgets[WIDX_CLOSE].left = w->width - 13;
    w->widgets[WIDX_CLOSE].right = w->width - 3;
    w->widgets[WIDX_LOCATE].right = w->width - 2;
    w->widgets[WIDX_LOCATE].left = w->width - 25;
    w->widgets[WIDX_KICK].right = w->width - 2;
    w->widgets[WIDX_KICK].left = w->width - 25;
    w->widgets[WIDX_VIEWPORT].right = w->width - 26;
    w->widgets[WIDX_VIEWPORT].bottom = w->height - 14;

    int32_t groupDropdownWidth = w->widgets[WIDX_GROUP].right - w->widgets[WIDX_GROUP].left;
    w->widgets[WIDX_GROUP].left = (w->width - groupDropdownWidth) / 2;
    w->widgets[WIDX_GROUP].right = w->widgets[WIDX_GROUP].left + groupDropdownWidth;
    w->widgets[WIDX_GROUP_DROPDOWN].left = w->widgets[WIDX_GROUP].right - 10;
    w->widgets[WIDX_GROUP_DROPDOWN].right = w->widgets[WIDX_GROUP].right;

    window_align_tabs(w, WIDX_TAB_1, WIDX_TAB_2);

    rct_viewport* viewport = w->viewport;
    if (viewport != nullptr)
    {
        rct_widget* viewportWidget = &window_player_overview_widgets[WIDX_VIEWPORT];

        viewport->pos = w->windowPos + ScreenCoordsXY{ viewportWidget->left, viewportWidget->top };
        viewport->width = viewportWidget->right - viewportWidget->left;
        viewport->height = viewportWidget->bottom - viewportWidget->top;
        viewport->view_width = viewport->width * viewport->zoom;
        viewport->view_height = viewport->height * viewport->zoom;
    }

    // Only enable kick button for other players
    const bool canKick = network_can_perform_action(network_get_current_player_group_index(), NETWORK_PERMISSION_KICK_PLAYER);
    const bool isServer = network_get_player_flags(playerIndex) & NETWORK_PLAYER_FLAG_ISSERVER;
    const bool isOwnWindow = (network_get_current_player_id() == w->number);
    widget_set_enabled(w, WIDX_KICK, canKick && !isOwnWindow && !isServer);
}

void window_player_statistics_close(rct_window* w)
{
    if (w->error.var_480)
    {
        w->error.var_480 = 0;
    }
}

void window_player_statistics_mouse_up(rct_window* w, rct_widgetindex widgetIndex)
{
    switch (widgetIndex)
    {
        case WIDX_CLOSE:
            window_close(w);
            break;
        case WIDX_TAB_1:
        case WIDX_TAB_2:
            window_player_set_page(w, widgetIndex - WIDX_TAB_1);
            break;
    }
}

void window_player_statistics_resize(rct_window* w)
{
    window_set_resize(w, 210, 80, 210, 80);
}

void window_player_statistics_update(rct_window* w)
{
    w->frame_no++;
    widget_invalidate(w, WIDX_TAB_1 + w->page);

    if (network_get_player_index((uint8_t)w->number) == -1)
    {
        window_close(w);
    }
}

void window_player_statistics_invalidate(rct_window* w)
{
    if (window_player_page_widgets[w->page] != w->widgets)
    {
        w->widgets = window_player_page_widgets[w->page];
        window_init_scroll_widgets(w);
    }

    w->pressed_widgets &= ~(WIDX_TAB_1);
    w->pressed_widgets &= ~(WIDX_TAB_2);
    w->pressed_widgets |= 1ULL << (w->page + WIDX_TAB_1);

    window_player_update_title(w);

    w->widgets[WIDX_BACKGROUND].right = w->width - 1;
    w->widgets[WIDX_BACKGROUND].bottom = w->height - 1;
    w->widgets[WIDX_PAGE_BACKGROUND].right = w->width - 1;
    w->widgets[WIDX_PAGE_BACKGROUND].bottom = w->height - 1;
    w->widgets[WIDX_TITLE].right = w->width - 2;
    w->widgets[WIDX_CLOSE].left = w->width - 13;
    w->widgets[WIDX_CLOSE].right = w->width - 3;

    window_align_tabs(w, WIDX_TAB_1, WIDX_TAB_2);
}

void window_player_statistics_paint(rct_window* w, rct_drawpixelinfo* dpi)
{
    window_draw_widgets(w, dpi);
    window_player_draw_tab_images(dpi, w);

    int32_t player = network_get_player_index((uint8_t)w->number);
    if (player == -1)
    {
        return;
    }

    int32_t x = w->windowPos.x + window_player_overview_widgets[WIDX_PAGE_BACKGROUND].left + 4;
    int32_t y = w->windowPos.y + window_player_overview_widgets[WIDX_PAGE_BACKGROUND].top + 4;

    set_format_arg(0, uint32_t, network_get_player_commands_ran(player));
    gfx_draw_string_left(dpi, STR_COMMANDS_RAN, gCommonFormatArgs, COLOUR_BLACK, x, y);

    y += LIST_ROW_HEIGHT;

    set_format_arg(0, uint32_t, network_get_player_money_spent(player));
    gfx_draw_string_left(dpi, STR_MONEY_SPENT, gCommonFormatArgs, COLOUR_BLACK, x, y);
}

static void window_player_set_page(rct_window* w, int32_t page)
{
    int32_t originalPage = w->page;

    w->page = page;
    w->frame_no = 0;
    w->no_list_items = 0;
    w->selected_list_item = -1;

    w->enabled_widgets = window_player_page_enabled_widgets[page];
    w->hold_down_widgets = 0;
    w->event_handlers = window_player_page_events[page];
    w->pressed_widgets = 0;
    w->widgets = window_player_page_widgets[page];
    w->Invalidate();
    window_event_resize_call(w);
    window_event_invalidate_call(w);
    window_init_scroll_widgets(w);
    w->Invalidate();

    if (page == WINDOW_PLAYER_PAGE_OVERVIEW)
    {
        if (w->viewport == nullptr)
        {
            viewport_create(
                w, w->windowPos, w->width, w->height, 0, TileCoordsXYZ(128, 128, 0).ToCoordsXYZ(), 1, SPRITE_INDEX_NULL);
            w->flags |= WF_NO_SCROLLING;
            window_event_invalidate_call(w);
            window_player_update_viewport(w, false);
        }
        else if (originalPage != page)
        {
            window_event_invalidate_call(w);
            window_player_update_viewport(w, false);
        }
    }
    else
    {
        if (w->viewport != nullptr)
        {
            w->viewport->width = 0;
            w->viewport = nullptr;
        }
    }
}

static void window_player_draw_tab_images(rct_drawpixelinfo* dpi, rct_window* w)
{
    rct_widget* widget;
    int32_t x, y, imageId;

    // Tab 1
    if (!widget_is_disabled(w, WIDX_TAB_1))
    {
        widget = &w->widgets[WIDX_TAB_1];
        x = widget->left + w->windowPos.x;
        y = widget->top + w->windowPos.y;
        imageId = SPR_PEEP_LARGE_FACE_NORMAL;
        gfx_draw_sprite(dpi, imageId, x, y, 0);
    }

    // Tab 2
    if (!widget_is_disabled(w, WIDX_TAB_2))
    {
        widget = &w->widgets[WIDX_TAB_2];
        x = widget->left + w->windowPos.x;
        y = widget->top + w->windowPos.y;
        imageId = SPR_TAB_FINANCES_SUMMARY_0;

        if (w->page == WINDOW_PLAYER_PAGE_STATISTICS)
        {
            imageId += (w->frame_no / 2) & 7;
        }

        gfx_draw_sprite(dpi, imageId, x, y, 0);
    }
}

static void window_player_update_viewport(rct_window* w, bool scroll)
{
    int32_t playerIndex = network_get_player_index((uint8_t)w->number);
    if (playerIndex == -1)
    {
        return;
    }

    rct_viewport* viewport = w->viewport;
    if (viewport != nullptr)
    {
        auto coord = network_get_player_last_action_coord(playerIndex);
        if (coord.x != 0 || coord.y != 0 || coord.z != 0)
        {
            auto centreLoc = centre_2d_coordinates(coord, viewport);
            if (!centreLoc)
            {
                return;
            }
            // Don't scroll if the view was originally undefined
            if (w->var_492 == -1)
            {
                scroll = false;
            }

            if (!scroll || w->savedViewPos != centreLoc)
            {
                w->flags |= WF_SCROLLING_TO_LOCATION;
                w->savedViewPos = *centreLoc;
                if (!scroll)
                {
                    w->viewport->viewPos = *centreLoc;
                }
                widget_invalidate(w, WIDX_VIEWPORT);
            }

            // Draw the viewport
            w->var_492 = 0;
        }
        else
        {
            // Don't draw the viewport
            w->var_492 = -1;
        }
    }
}

static void window_player_update_title(rct_window* w)
{
    int32_t player = network_get_player_index((uint8_t)w->number);
    if (player != -1)
    {
        set_format_arg(0, const char*, network_get_player_name(player)); // set title caption to player name
    }
    else
    {
        set_format_arg(0, const char*, "");
    }
}
