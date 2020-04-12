/*****************************************************************************
 * Copyright (c) 2014-2019 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "../interface/Theme.h"

#include <algorithm>
#include <openrct2-ui/interface/Widget.h>
#include <openrct2-ui/windows/Window.h>
#include <openrct2/Context.h>
#include <openrct2/Game.h>
#include <openrct2/Input.h>
#include <openrct2/OpenRCT2.h>
#include <openrct2/config/Config.h>
#include <openrct2/localisation/Date.h>
#include <openrct2/localisation/Localisation.h>
#include <openrct2/management/Finance.h>
#include <openrct2/management/NewsItem.h>
#include <openrct2/peep/Staff.h>
#include <openrct2/sprites.h>
#include <openrct2/world/Climate.h>
#include <openrct2/world/Park.h>
#include <openrct2/world/Sprite.h>

// clang-format off
enum WINDOW_GAME_BOTTOM_TOOLBAR_WIDGET_IDX
{
    WIDX_LEFT_OUTSET,
    WIDX_LEFT_INSET,
    WIDX_MONEY,
    WIDX_GUESTS,
    WIDX_PARK_RATING,

    WIDX_MIDDLE_OUTSET,
    WIDX_MIDDLE_INSET,
    WIDX_NEWS_SUBJECT,
    WIDX_NEWS_LOCATE,

    WIDX_RIGHT_OUTSET,
    WIDX_RIGHT_INSET,
    WIDX_DATE
};

static rct_widget window_game_bottom_toolbar_widgets[] =
{
    { WWT_IMGBTN,   0,    0,   141,    0,   33,   0xFFFFFFFF,   STR_NONE },                               // Left outset panel
    { WWT_IMGBTN,   0,    2,   139,    2,   31,   0xFFFFFFFF,   STR_NONE },                               // Left inset panel
    { WWT_FLATBTN,  0,    2,   139,    1,   12,   0xFFFFFFFF,   STR_PROFIT_PER_WEEK_AND_PARK_VALUE_TIP }, // Money window
    { WWT_FLATBTN,  0,    2,   139,   11,   22,   0xFFFFFFFF,   STR_NONE },                               // Guests window
    { WWT_FLATBTN,  0,    2,   139,   21,   31,   0xFFFFFFFF,   STR_PARK_RATING_TIP },                    // Park rating window

    { WWT_IMGBTN,   2,  142,   497,    0,   33,   0xFFFFFFFF,   STR_NONE },               // Middle outset panel
    { WWT_PLACEHOLDER,    2,  144,   495,    2,   31,   0xFFFFFFFF,   STR_NONE },         // Middle inset panel
    { WWT_FLATBTN,  2,  147,   170,    5,   28,   0xFFFFFFFF,   STR_SHOW_SUBJECT_TIP },   // Associated news item window
    { WWT_FLATBTN,  2,  469,   492,    5,   28,   SPR_LOCATE,   STR_LOCATE_SUBJECT_TIP }, // Scroll to news item target

    { WWT_IMGBTN,   0,  498,   639,    0,   33,   0xFFFFFFFF,   STR_NONE }, // Right outset panel
    { WWT_IMGBTN,   0,  500,   637,    2,   31,   0xFFFFFFFF,   STR_NONE }, // Right inset panel
    { WWT_FLATBTN,  0,  500,   637,    2,   13,   0xFFFFFFFF,   STR_NONE }, // Date
    { WIDGETS_END },
};

uint8_t gToolbarDirtyFlags;

static void window_game_bottom_toolbar_mouseup(rct_window *w, rct_widgetindex widgetIndex);
static void window_game_bottom_toolbar_tooltip(rct_window* w, rct_widgetindex widgetIndex, rct_string_id *stringId);
static void window_game_bottom_toolbar_invalidate(rct_window *w);
static void window_game_bottom_toolbar_paint(rct_window *w, rct_drawpixelinfo *dpi);
static void window_game_bottom_toolbar_update(rct_window* w);
static void window_game_bottom_toolbar_cursor(rct_window *w, rct_widgetindex widgetIndex, const ScreenCoordsXY& screenCoords, int32_t *cursorId);
static void window_game_bottom_toolbar_unknown05(rct_window *w);

static void window_game_bottom_toolbar_draw_left_panel(rct_drawpixelinfo *dpi, rct_window *w);
static void window_game_bottom_toolbar_draw_park_rating(rct_drawpixelinfo *dpi, rct_window *w, int32_t colour, int32_t x, int32_t y, uint8_t factor);
static void window_game_bottom_toolbar_draw_right_panel(rct_drawpixelinfo *dpi, rct_window *w);
static void window_game_bottom_toolbar_draw_news_item(rct_drawpixelinfo *dpi, rct_window *w);
static void window_game_bottom_toolbar_draw_middle_panel(rct_drawpixelinfo *dpi, rct_window *w);

/**
 *
 *  rct2: 0x0097BFDC
 */
static rct_window_event_list window_game_bottom_toolbar_events =
{
    nullptr,
    window_game_bottom_toolbar_mouseup,
    nullptr,
    nullptr,
    nullptr,
    window_game_bottom_toolbar_unknown05,
    window_game_bottom_toolbar_update,
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
    window_game_bottom_toolbar_tooltip,
    window_game_bottom_toolbar_cursor,
    nullptr,
    window_game_bottom_toolbar_invalidate,
    window_game_bottom_toolbar_paint,
    nullptr
};
// clang-format on

static void window_game_bottom_toolbar_invalidate_dirty_widgets(rct_window* w);

/**
 * Creates the main game bottom toolbar window.
 *  rct2: 0x0066B52F (part of 0x0066B3E8)
 */
rct_window* window_game_bottom_toolbar_open()
{
    int32_t screenWidth = context_get_width();
    int32_t screenHeight = context_get_height();

    // Figure out how much line height we have to work with.
    uint32_t line_height = font_get_line_height(FONT_SPRITE_BASE_MEDIUM);
    uint32_t toolbar_height = line_height * 2 + 12;

    rct_window* window = window_create(
        ScreenCoordsXY(0, screenHeight - toolbar_height), screenWidth, toolbar_height, &window_game_bottom_toolbar_events,
        WC_BOTTOM_TOOLBAR, WF_STICK_TO_FRONT | WF_TRANSPARENT | WF_NO_BACKGROUND);
    window->widgets = window_game_bottom_toolbar_widgets;
    window->enabled_widgets |= (1 << WIDX_LEFT_OUTSET) | (1 << WIDX_MONEY) | (1 << WIDX_GUESTS) | (1 << WIDX_PARK_RATING)
        | (1 << WIDX_MIDDLE_OUTSET) | (1 << WIDX_MIDDLE_INSET) | (1 << WIDX_NEWS_SUBJECT) | (1 << WIDX_NEWS_LOCATE)
        | (1 << WIDX_RIGHT_OUTSET) | (1 << WIDX_DATE);

    window->frame_no = 0;
    window_init_scroll_widgets(window);

    // Reset the middle widget to not show by default.
    // If it is required to be shown news_update will reshow it.
    window_game_bottom_toolbar_widgets[WIDX_MIDDLE_OUTSET].type = WWT_EMPTY;

    return window;
}

/**
 *
 *  rct2: 0x0066C588
 */
static void window_game_bottom_toolbar_mouseup(rct_window* w, rct_widgetindex widgetIndex)
{
    NewsItem* newsItem;

    switch (widgetIndex)
    {
        case WIDX_LEFT_OUTSET:
        case WIDX_MONEY:
            if (!(gParkFlags & PARK_FLAGS_NO_MONEY))
                context_open_window(WC_FINANCES);
            break;
        case WIDX_GUESTS:
            context_open_window_view(WV_PARK_GUESTS);
            break;
        case WIDX_PARK_RATING:
            context_open_window_view(WV_PARK_RATING);
            break;
        case WIDX_MIDDLE_INSET:
            if (news_item_is_queue_empty())
            {
                context_open_window(WC_RECENT_NEWS);
            }
            else
            {
                news_item_close_current();
            }
            break;
        case WIDX_NEWS_SUBJECT:
            newsItem = news_item_get(0);
            news_item_open_subject(newsItem->Type, newsItem->Assoc);
            break;
        case WIDX_NEWS_LOCATE:
            if (news_item_is_queue_empty())
                break;

            {
                newsItem = news_item_get(0);

                auto subjectLoc = news_item_get_subject_location(newsItem->Type, newsItem->Assoc);

                if (subjectLoc == std::nullopt)
                    break;

                rct_window* mainWindow = window_get_main();
                if (mainWindow != nullptr)
                    window_scroll_to_location(mainWindow, subjectLoc->x, subjectLoc->y, subjectLoc->z);
            }
            break;
        case WIDX_RIGHT_OUTSET:
        case WIDX_DATE:
            context_open_window(WC_RECENT_NEWS);
            break;
    }
}

static void window_game_bottom_toolbar_tooltip(rct_window* w, rct_widgetindex widgetIndex, rct_string_id* stringId)
{
    int32_t month, day;

    switch (widgetIndex)
    {
        case WIDX_MONEY:
            set_format_arg(0, int32_t, gCurrentProfit);
            set_format_arg(4, int32_t, gParkValue);
            break;
        case WIDX_PARK_RATING:
            set_format_arg(0, int16_t, gParkRating);
            break;
        case WIDX_DATE:
            month = date_get_month(gDateMonthsElapsed);
            day = ((gDateMonthTicks * days_in_month[month]) >> 16) & 0xFF;

            set_format_arg(0, rct_string_id, DateDayNames[day]);
            set_format_arg(2, rct_string_id, DateGameMonthNames[month]);
            break;
    }
}

/**
 *
 *  rct2: 0x0066BBA0
 */
static void window_game_bottom_toolbar_invalidate(rct_window* w)
{
    // Figure out how much line height we have to work with.
    uint32_t line_height = font_get_line_height(FONT_SPRITE_BASE_MEDIUM);

    // Reset dimensions as appropriate -- in case we're switching languages.
    w->height = line_height * 2 + 12;
    w->windowPos.y = context_get_height() - w->height;

    // Change height of widgets in accordance with line height.
    w->widgets[WIDX_LEFT_OUTSET].bottom = w->widgets[WIDX_MIDDLE_OUTSET].bottom = w->widgets[WIDX_RIGHT_OUTSET].bottom
        = line_height * 3 + 3;
    w->widgets[WIDX_LEFT_INSET].bottom = w->widgets[WIDX_MIDDLE_INSET].bottom = w->widgets[WIDX_RIGHT_INSET].bottom
        = line_height * 3 + 1;

    // Reposition left widgets in accordance with line height... depending on whether there is money in play.
    if (gParkFlags & PARK_FLAGS_NO_MONEY)
    {
        w->widgets[WIDX_MONEY].type = WWT_EMPTY;
        w->widgets[WIDX_GUESTS].top = 1;
        w->widgets[WIDX_GUESTS].bottom = line_height + 7;
        w->widgets[WIDX_PARK_RATING].top = line_height + 8;
        w->widgets[WIDX_PARK_RATING].bottom = w->height - 1;
    }
    else
    {
        w->widgets[WIDX_MONEY].type = WWT_FLATBTN;
        w->widgets[WIDX_MONEY].bottom = w->widgets[WIDX_MONEY].top + line_height;
        w->widgets[WIDX_GUESTS].top = w->widgets[WIDX_MONEY].bottom + 1;
        w->widgets[WIDX_GUESTS].bottom = w->widgets[WIDX_GUESTS].top + line_height;
        w->widgets[WIDX_PARK_RATING].top = w->widgets[WIDX_GUESTS].bottom - 1;
        w->widgets[WIDX_PARK_RATING].bottom = w->height - 1;
    }

    // Reposition right widgets in accordance with line height, too.
    w->widgets[WIDX_DATE].bottom = line_height + 1;

    // Anchor the middle and right panel to the right
    int32_t x = context_get_width();
    w->width = x;
    x--;
    window_game_bottom_toolbar_widgets[WIDX_RIGHT_OUTSET].right = x;
    x -= 2;
    window_game_bottom_toolbar_widgets[WIDX_RIGHT_INSET].right = x;
    x -= 137;
    window_game_bottom_toolbar_widgets[WIDX_RIGHT_INSET].left = x;
    x -= 2;
    window_game_bottom_toolbar_widgets[WIDX_RIGHT_OUTSET].left = x;
    x--;
    window_game_bottom_toolbar_widgets[WIDX_MIDDLE_OUTSET].right = x;
    x -= 2;
    window_game_bottom_toolbar_widgets[WIDX_MIDDLE_INSET].right = x;
    x -= 3;
    window_game_bottom_toolbar_widgets[WIDX_NEWS_LOCATE].right = x;
    x -= 23;
    window_game_bottom_toolbar_widgets[WIDX_NEWS_LOCATE].left = x;
    window_game_bottom_toolbar_widgets[WIDX_DATE].left = window_game_bottom_toolbar_widgets[WIDX_RIGHT_OUTSET].left + 2;
    window_game_bottom_toolbar_widgets[WIDX_DATE].right = window_game_bottom_toolbar_widgets[WIDX_RIGHT_OUTSET].right - 2;

    window_game_bottom_toolbar_widgets[WIDX_LEFT_INSET].type = WWT_EMPTY;
    window_game_bottom_toolbar_widgets[WIDX_RIGHT_INSET].type = WWT_EMPTY;

    if (news_item_is_queue_empty())
    {
        if (!(theme_get_flags() & UITHEME_FLAG_USE_FULL_BOTTOM_TOOLBAR))
        {
            window_game_bottom_toolbar_widgets[WIDX_MIDDLE_OUTSET].type = WWT_EMPTY;
            window_game_bottom_toolbar_widgets[WIDX_MIDDLE_INSET].type = WWT_EMPTY;
            window_game_bottom_toolbar_widgets[WIDX_NEWS_SUBJECT].type = WWT_EMPTY;
            window_game_bottom_toolbar_widgets[WIDX_NEWS_LOCATE].type = WWT_EMPTY;
        }
        else
        {
            window_game_bottom_toolbar_widgets[WIDX_MIDDLE_OUTSET].type = WWT_IMGBTN;
            window_game_bottom_toolbar_widgets[WIDX_MIDDLE_INSET].type = WWT_PLACEHOLDER;
            window_game_bottom_toolbar_widgets[WIDX_NEWS_SUBJECT].type = WWT_EMPTY;
            window_game_bottom_toolbar_widgets[WIDX_NEWS_LOCATE].type = WWT_EMPTY;
            window_game_bottom_toolbar_widgets[WIDX_MIDDLE_OUTSET].colour = 0;
            window_game_bottom_toolbar_widgets[WIDX_MIDDLE_INSET].colour = 0;
        }
    }
    else
    {
        NewsItem* newsItem = news_item_get(0);
        window_game_bottom_toolbar_widgets[WIDX_MIDDLE_OUTSET].type = WWT_IMGBTN;
        window_game_bottom_toolbar_widgets[WIDX_MIDDLE_INSET].type = WWT_PLACEHOLDER;
        window_game_bottom_toolbar_widgets[WIDX_NEWS_SUBJECT].type = WWT_FLATBTN;
        window_game_bottom_toolbar_widgets[WIDX_NEWS_LOCATE].type = WWT_FLATBTN;
        window_game_bottom_toolbar_widgets[WIDX_MIDDLE_OUTSET].colour = 2;
        window_game_bottom_toolbar_widgets[WIDX_MIDDLE_INSET].colour = 2;
        w->disabled_widgets &= ~(1 << WIDX_NEWS_SUBJECT);
        w->disabled_widgets &= ~(1 << WIDX_NEWS_LOCATE);

        // Find out if the news item is no longer valid
        auto subjectLoc = news_item_get_subject_location(newsItem->Type, newsItem->Assoc);

        if (subjectLoc == std::nullopt)
            w->disabled_widgets |= (1 << WIDX_NEWS_LOCATE);

        if (!(news_type_properties[newsItem->Type] & NEWS_TYPE_HAS_SUBJECT))
        {
            w->disabled_widgets |= (1 << WIDX_NEWS_SUBJECT);
            window_game_bottom_toolbar_widgets[WIDX_NEWS_SUBJECT].type = WWT_EMPTY;
        }

        if (newsItem->Flags & NEWS_FLAG_HAS_BUTTON)
        {
            w->disabled_widgets |= (1 << WIDX_NEWS_SUBJECT);
            w->disabled_widgets |= (1 << WIDX_NEWS_LOCATE);
        }
    }
}

/**
 *
 *  rct2: 0x0066BB79
 */
void window_game_bottom_toolbar_invalidate_news_item()
{
    if (gScreenFlags == SCREEN_FLAGS_PLAYING)
    {
        widget_invalidate_by_class(WC_BOTTOM_TOOLBAR, WIDX_MIDDLE_OUTSET);
    }
}

/**
 *
 *  rct2: 0x0066BC87
 */
static void window_game_bottom_toolbar_paint(rct_window* w, rct_drawpixelinfo* dpi)
{
    // Draw panel grey backgrounds
    gfx_filter_rect(
        dpi, w->windowPos.x + window_game_bottom_toolbar_widgets[WIDX_LEFT_OUTSET].left,
        w->windowPos.y + window_game_bottom_toolbar_widgets[WIDX_LEFT_OUTSET].top,
        w->windowPos.x + window_game_bottom_toolbar_widgets[WIDX_LEFT_OUTSET].right,
        w->windowPos.y + window_game_bottom_toolbar_widgets[WIDX_LEFT_OUTSET].bottom, PALETTE_51);
    gfx_filter_rect(
        dpi, w->windowPos.x + window_game_bottom_toolbar_widgets[WIDX_RIGHT_OUTSET].left,
        w->windowPos.y + window_game_bottom_toolbar_widgets[WIDX_RIGHT_OUTSET].top,
        w->windowPos.x + window_game_bottom_toolbar_widgets[WIDX_RIGHT_OUTSET].right,
        w->windowPos.y + window_game_bottom_toolbar_widgets[WIDX_RIGHT_OUTSET].bottom, PALETTE_51);

    if (theme_get_flags() & UITHEME_FLAG_USE_FULL_BOTTOM_TOOLBAR)
    {
        // Draw grey background
        gfx_filter_rect(
            dpi, w->windowPos.x + window_game_bottom_toolbar_widgets[WIDX_MIDDLE_OUTSET].left,
            w->windowPos.y + window_game_bottom_toolbar_widgets[WIDX_MIDDLE_OUTSET].top,
            w->windowPos.x + window_game_bottom_toolbar_widgets[WIDX_MIDDLE_OUTSET].right,
            w->windowPos.y + window_game_bottom_toolbar_widgets[WIDX_MIDDLE_OUTSET].bottom, PALETTE_51);
    }

    window_draw_widgets(w, dpi);

    window_game_bottom_toolbar_draw_left_panel(dpi, w);
    window_game_bottom_toolbar_draw_right_panel(dpi, w);

    if (!news_item_is_queue_empty())
    {
        window_game_bottom_toolbar_draw_news_item(dpi, w);
    }
    else if (theme_get_flags() & UITHEME_FLAG_USE_FULL_BOTTOM_TOOLBAR)
    {
        window_game_bottom_toolbar_draw_middle_panel(dpi, w);
    }
}

static void window_game_bottom_toolbar_draw_left_panel(rct_drawpixelinfo* dpi, rct_window* w)
{
    // Draw green inset rectangle on panel
    gfx_fill_rect_inset(
        dpi, w->windowPos.x + window_game_bottom_toolbar_widgets[WIDX_LEFT_OUTSET].left + 1,
        w->windowPos.y + window_game_bottom_toolbar_widgets[WIDX_LEFT_OUTSET].top + 1,
        w->windowPos.x + window_game_bottom_toolbar_widgets[WIDX_LEFT_OUTSET].right - 1,
        w->windowPos.y + window_game_bottom_toolbar_widgets[WIDX_LEFT_OUTSET].bottom - 1, w->colours[1], INSET_RECT_F_30);

    // Figure out how much line height we have to work with.
    uint32_t line_height = font_get_line_height(FONT_SPRITE_BASE_MEDIUM);

    // Draw money
    if (!(gParkFlags & PARK_FLAGS_NO_MONEY))
    {
        rct_widget widget = window_game_bottom_toolbar_widgets[WIDX_MONEY];
        int32_t x = w->windowPos.x + (widget.left + widget.right) / 2;
        int32_t y = w->windowPos.y + (widget.top + widget.bottom) / 2 - (line_height == 10 ? 5 : 6);

        set_format_arg(0, money32, gCash);
        gfx_draw_string_centred(
            dpi, (gCash < 0 ? STR_BOTTOM_TOOLBAR_CASH_NEGATIVE : STR_BOTTOM_TOOLBAR_CASH), x, y,
            (gHoverWidget.window_classification == WC_BOTTOM_TOOLBAR && gHoverWidget.widget_index == WIDX_MONEY
                 ? COLOUR_WHITE
                 : NOT_TRANSLUCENT(w->colours[0])),
            gCommonFormatArgs);
    }

    static constexpr const rct_string_id guestCountFormats[] = {
        STR_BOTTOM_TOOLBAR_NUM_GUESTS_STABLE,
        STR_BOTTOM_TOOLBAR_NUM_GUESTS_DECREASE,
        STR_BOTTOM_TOOLBAR_NUM_GUESTS_INCREASE,
    };

    static constexpr const rct_string_id guestCountFormatsSingular[] = {
        STR_BOTTOM_TOOLBAR_NUM_GUESTS_STABLE_SINGULAR,
        STR_BOTTOM_TOOLBAR_NUM_GUESTS_DECREASE_SINGULAR,
        STR_BOTTOM_TOOLBAR_NUM_GUESTS_INCREASE_SINGULAR,
    };

    // Draw guests
    {
        rct_widget widget = window_game_bottom_toolbar_widgets[WIDX_GUESTS];
        int32_t x = w->windowPos.x + (widget.left + widget.right) / 2;
        int32_t y = w->windowPos.y + (widget.top + widget.bottom) / 2 - 6;

        gfx_draw_string_centred(
            dpi,
            gNumGuestsInPark == 1 ? guestCountFormatsSingular[gGuestChangeModifier] : guestCountFormats[gGuestChangeModifier],
            x, y,
            (gHoverWidget.window_classification == WC_BOTTOM_TOOLBAR && gHoverWidget.widget_index == WIDX_GUESTS
                 ? COLOUR_WHITE
                 : NOT_TRANSLUCENT(w->colours[0])),
            &gNumGuestsInPark);
    }

    // Draw park rating
    {
        rct_widget widget = window_game_bottom_toolbar_widgets[WIDX_PARK_RATING];
        int32_t x = w->windowPos.x + widget.left + 11;
        int32_t y = w->windowPos.y + (widget.top + widget.bottom) / 2 - 5;

        window_game_bottom_toolbar_draw_park_rating(dpi, w, w->colours[3], x, y, std::max(10, ((gParkRating / 4) * 263) / 256));
    }
}

/**
 *
 *  rct2: 0x0066C76C
 */
static void window_game_bottom_toolbar_draw_park_rating(
    rct_drawpixelinfo* dpi, rct_window* w, int32_t colour, int32_t x, int32_t y, uint8_t factor)
{
    int16_t bar_width;

    bar_width = (factor * 114) / 255;
    gfx_fill_rect_inset(dpi, x + 1, y + 1, x + 114, y + 9, w->colours[1], INSET_RECT_F_30);
    if (!(colour & IMAGE_TYPE_REMAP_2_PLUS) || game_is_paused() || (gCurrentRealTimeTicks & 8))
    {
        if (bar_width > 2)
            gfx_fill_rect_inset(dpi, x + 2, y + 2, x + bar_width - 1, y + 8, colour & 0x7FFFFFFF, 0);
    }

    // Draw thumbs on the sides
    gfx_draw_sprite(dpi, SPR_RATING_LOW, x - 14, y, 0);
    gfx_draw_sprite(dpi, SPR_RATING_HIGH, x + 114, y, 0);
}

static void window_game_bottom_toolbar_draw_right_panel(rct_drawpixelinfo* dpi, rct_window* w)
{
    // Draw green inset rectangle on panel
    gfx_fill_rect_inset(
        dpi, w->windowPos.x + window_game_bottom_toolbar_widgets[WIDX_RIGHT_OUTSET].left + 1,
        w->windowPos.y + window_game_bottom_toolbar_widgets[WIDX_RIGHT_OUTSET].top + 1,
        w->windowPos.x + window_game_bottom_toolbar_widgets[WIDX_RIGHT_OUTSET].right - 1,
        w->windowPos.y + window_game_bottom_toolbar_widgets[WIDX_RIGHT_OUTSET].bottom - 1, w->colours[1], INSET_RECT_F_30);

    int32_t x = (window_game_bottom_toolbar_widgets[WIDX_RIGHT_OUTSET].left
                 + window_game_bottom_toolbar_widgets[WIDX_RIGHT_OUTSET].right)
            / 2
        + w->windowPos.x;
    int32_t y = window_game_bottom_toolbar_widgets[WIDX_RIGHT_OUTSET].top + w->windowPos.y + 2;

    // Date
    int32_t year = date_get_year(gDateMonthsElapsed) + 1;
    int32_t month = date_get_month(gDateMonthsElapsed);
    int32_t day = ((gDateMonthTicks * days_in_month[month]) >> 16) & 0xFF;

    rct_string_id stringId = DateFormatStringFormatIds[gConfigGeneral.date_format];
    set_format_arg(0, rct_string_id, DateDayNames[day]);
    set_format_arg(2, int16_t, month);
    set_format_arg(4, int16_t, year);
    gfx_draw_string_centred(
        dpi, stringId, x, y,
        (gHoverWidget.window_classification == WC_BOTTOM_TOOLBAR && gHoverWidget.widget_index == WIDX_DATE
             ? COLOUR_WHITE
             : NOT_TRANSLUCENT(w->colours[0])),
        gCommonFormatArgs);

    // Figure out how much line height we have to work with.
    uint32_t line_height = font_get_line_height(FONT_SPRITE_BASE_MEDIUM);

    // Temperature
    x = w->windowPos.x + window_game_bottom_toolbar_widgets[WIDX_RIGHT_OUTSET].left + 15;
    y += line_height + 1;

    int32_t temperature = gClimateCurrent.Temperature;
    rct_string_id format = STR_CELSIUS_VALUE;
    if (gConfigGeneral.temperature_format == TEMPERATURE_FORMAT_F)
    {
        temperature = climate_celsius_to_fahrenheit(temperature);
        format = STR_FAHRENHEIT_VALUE;
    }
    set_format_arg(0, int16_t, temperature);
    gfx_draw_string_left(dpi, format, gCommonFormatArgs, COLOUR_BLACK, x, y + 6);
    x += 30;

    // Current weather
    auto currentWeatherSpriteId = climate_get_weather_sprite_id(gClimateCurrent);
    gfx_draw_sprite(dpi, currentWeatherSpriteId, x, y, 0);

    // Next weather
    auto nextWeatherSpriteId = climate_get_weather_sprite_id(gClimateNext);
    if (currentWeatherSpriteId != nextWeatherSpriteId)
    {
        if (gClimateUpdateTimer < 960)
        {
            gfx_draw_sprite(dpi, SPR_NEXT_WEATHER, x + 27, y + 5, 0);
            gfx_draw_sprite(dpi, nextWeatherSpriteId, x + 40, y, 0);
        }
    }
}

/**
 *
 *  rct2: 0x0066BFA5
 */
static void window_game_bottom_toolbar_draw_news_item(rct_drawpixelinfo* dpi, rct_window* w)
{
    int32_t x, y, width;
    NewsItem* newsItem;
    rct_widget* middleOutsetWidget;

    middleOutsetWidget = &window_game_bottom_toolbar_widgets[WIDX_MIDDLE_OUTSET];
    newsItem = news_item_get(0);

    // Current news item
    gfx_fill_rect_inset(
        dpi, w->windowPos.x + middleOutsetWidget->left + 1, w->windowPos.y + middleOutsetWidget->top + 1,
        w->windowPos.x + middleOutsetWidget->right - 1, w->windowPos.y + middleOutsetWidget->bottom - 1, w->colours[2],
        INSET_RECT_F_30);

    // Text
    utf8* newsItemText = newsItem->Text;
    x = w->windowPos.x + (middleOutsetWidget->left + middleOutsetWidget->right) / 2;
    y = w->windowPos.y + middleOutsetWidget->top + 11;
    width = middleOutsetWidget->right - middleOutsetWidget->left - 62;
    gfx_draw_string_centred_wrapped_partial(
        dpi, x, y, width, COLOUR_BRIGHT_GREEN, STR_BOTTOM_TOOLBAR_NEWS_TEXT, &newsItemText, newsItem->Ticks);

    x = w->windowPos.x + window_game_bottom_toolbar_widgets[WIDX_NEWS_SUBJECT].left;
    y = w->windowPos.y + window_game_bottom_toolbar_widgets[WIDX_NEWS_SUBJECT].top;
    switch (newsItem->Type)
    {
        case NEWS_ITEM_RIDE:
            gfx_draw_sprite(dpi, SPR_RIDE, x, y, 0);
            break;
        case NEWS_ITEM_PEEP_ON_RIDE:
        case NEWS_ITEM_PEEP:
        {
            if (newsItem->Flags & NEWS_FLAG_HAS_BUTTON)
                break;

            rct_drawpixelinfo cliped_dpi;
            if (!clip_drawpixelinfo(&cliped_dpi, dpi, x + 1, y + 1, 22, 22))
            {
                break;
            }

            Peep* peep = GET_PEEP(newsItem->Assoc);
            int32_t clip_x = 10, clip_y = 19;

            if (peep->type == PEEP_TYPE_STAFF && peep->staff_type == STAFF_TYPE_ENTERTAINER)
            {
                clip_y += 3;
            }

            uint32_t image_id_base = g_peep_animation_entries[peep->sprite_type].sprite_animation->base_image;
            image_id_base += w->frame_no & 0xFFFFFFFC;
            image_id_base++;

            uint32_t image_id = image_id_base;
            image_id |= SPRITE_ID_PALETTE_COLOUR_2(peep->tshirt_colour, peep->trousers_colour);

            gfx_draw_sprite(&cliped_dpi, image_id, clip_x, clip_y, 0);

            if (image_id_base >= 0x2A1D && image_id_base < 0x2A3D)
            {
                image_id_base += 32;
                image_id_base |= SPRITE_ID_PALETTE_COLOUR_1(peep->balloon_colour);

                gfx_draw_sprite(&cliped_dpi, image_id_base, clip_x, clip_y, 0);
            }
            else if (image_id_base >= 0x2BBD && image_id_base < 0x2BDD)
            {
                image_id_base += 32;
                image_id_base |= SPRITE_ID_PALETTE_COLOUR_1(peep->umbrella_colour);

                gfx_draw_sprite(&cliped_dpi, image_id_base, clip_x, clip_y, 0);
            }
            else if (image_id_base >= 0x29DD && image_id_base < 0x29FD)
            {
                image_id_base += 32;
                image_id_base |= SPRITE_ID_PALETTE_COLOUR_1(peep->hat_colour);

                gfx_draw_sprite(&cliped_dpi, image_id_base, clip_x, clip_y, 0);
            }
            break;
        }
        case NEWS_ITEM_MONEY:
            gfx_draw_sprite(dpi, SPR_FINANCE, x, y, 0);
            break;
        case NEWS_ITEM_RESEARCH:
            gfx_draw_sprite(dpi, (newsItem->Assoc < 0x10000 ? SPR_NEW_SCENERY : SPR_NEW_RIDE), x, y, 0);
            break;
        case NEWS_ITEM_PEEPS:
            gfx_draw_sprite(dpi, SPR_GUESTS, x, y, 0);
            break;
        case NEWS_ITEM_AWARD:
            gfx_draw_sprite(dpi, SPR_AWARD, x, y, 0);
            break;
        case NEWS_ITEM_GRAPH:
            gfx_draw_sprite(dpi, SPR_GRAPH, x, y, 0);
            break;
    }
}

static void window_game_bottom_toolbar_draw_middle_panel(rct_drawpixelinfo* dpi, rct_window* w)
{
    rct_widget* middleOutsetWidget = &window_game_bottom_toolbar_widgets[WIDX_MIDDLE_OUTSET];

    gfx_fill_rect_inset(
        dpi, w->windowPos.x + middleOutsetWidget->left + 1, w->windowPos.y + middleOutsetWidget->top + 1,
        w->windowPos.x + middleOutsetWidget->right - 1, w->windowPos.y + middleOutsetWidget->bottom - 1, w->colours[1],
        INSET_RECT_F_30);

    // Figure out how much line height we have to work with.
    uint32_t line_height = font_get_line_height(FONT_SPRITE_BASE_MEDIUM);

    int32_t x = w->windowPos.x + (middleOutsetWidget->left + middleOutsetWidget->right) / 2;
    int32_t y = w->windowPos.y + middleOutsetWidget->top + line_height + 1;
    int32_t width = middleOutsetWidget->right - middleOutsetWidget->left - 62;

    // Check if there is a map tooltip to draw
    rct_string_id stringId;
    std::memcpy(&stringId, gMapTooltipFormatArgs, sizeof(rct_string_id));
    if (stringId == STR_NONE)
    {
        gfx_draw_string_centred_wrapped(dpi, gMapTooltipFormatArgs, x, y, width, STR_TITLE_SEQUENCE_OPENRCT2, w->colours[0]);
    }
    else
    {
        // Show tooltip in bottom toolbar
        gfx_draw_string_centred_wrapped(dpi, gMapTooltipFormatArgs, x, y, width, STR_STRINGID, w->colours[0]);
    }
}

/**
 *
 *  rct2: 0x0066C6D8
 */
static void window_game_bottom_toolbar_update(rct_window* w)
{
    w->frame_no++;
    if (w->frame_no >= 24)
        w->frame_no = 0;

    window_game_bottom_toolbar_invalidate_dirty_widgets(w);
}

/**
 *
 *  rct2: 0x0066C644
 */
static void window_game_bottom_toolbar_cursor(
    rct_window* w, rct_widgetindex widgetIndex, const ScreenCoordsXY& screenCoords, int32_t* cursorId)
{
    switch (widgetIndex)
    {
        case WIDX_MONEY:
        case WIDX_GUESTS:
        case WIDX_PARK_RATING:
        case WIDX_DATE:
            gTooltipTimeout = 2000;
            break;
    }
}

/**
 *
 *  rct2: 0x0066C6F2
 */
static void window_game_bottom_toolbar_unknown05(rct_window* w)
{
    window_game_bottom_toolbar_invalidate_dirty_widgets(w);
}

/**
 *
 *  rct2: 0x0066C6F2
 */
static void window_game_bottom_toolbar_invalidate_dirty_widgets(rct_window* w)
{
    if (gToolbarDirtyFlags & BTM_TB_DIRTY_FLAG_MONEY)
    {
        gToolbarDirtyFlags &= ~BTM_TB_DIRTY_FLAG_MONEY;
        widget_invalidate(w, WIDX_LEFT_INSET);
    }

    if (gToolbarDirtyFlags & BTM_TB_DIRTY_FLAG_DATE)
    {
        gToolbarDirtyFlags &= ~BTM_TB_DIRTY_FLAG_DATE;
        widget_invalidate(w, WIDX_RIGHT_INSET);
    }

    if (gToolbarDirtyFlags & BTM_TB_DIRTY_FLAG_PEEP_COUNT)
    {
        gToolbarDirtyFlags &= ~BTM_TB_DIRTY_FLAG_PEEP_COUNT;
        widget_invalidate(w, WIDX_LEFT_INSET);
    }

    if (gToolbarDirtyFlags & BTM_TB_DIRTY_FLAG_CLIMATE)
    {
        gToolbarDirtyFlags &= ~BTM_TB_DIRTY_FLAG_CLIMATE;
        widget_invalidate(w, WIDX_RIGHT_INSET);
    }

    if (gToolbarDirtyFlags & BTM_TB_DIRTY_FLAG_PARK_RATING)
    {
        gToolbarDirtyFlags &= ~BTM_TB_DIRTY_FLAG_PARK_RATING;
        widget_invalidate(w, WIDX_LEFT_INSET);
    }
}
