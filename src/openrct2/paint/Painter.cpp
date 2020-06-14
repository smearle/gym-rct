/*****************************************************************************
 * Copyright (c) 2014-2019 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "Painter.h"

#include "../Game.h"
#include "../Intro.h"
#include "../OpenRCT2.h"
#include "../ReplayManager.h"
#include "../config/Config.h"
#include "../drawing/Drawing.h"
#include "../drawing/IDrawingEngine.h"
#include "../interface/Chat.h"
#include "../interface/InteractiveConsole.h"
#include "../localisation/FormatCodes.h"
#include "../localisation/Language.h"
#include "../paint/Paint.h"
#include "../title/TitleScreen.h"
#include "../ui/UiContext.h"

using namespace OpenRCT2;
using namespace OpenRCT2::Drawing;
using namespace OpenRCT2::Paint;
using namespace OpenRCT2::Ui;

Painter::Painter(const std::shared_ptr<IUiContext>& uiContext)
    : _uiContext(uiContext)
{
}

void Painter::Paint(IDrawingEngine& de)
{
    auto dpi = de.GetDrawingPixelInfo();
    if (gIntroState != INTRO_STATE_NONE)
    {
        intro_draw(dpi);
    }
    else
    {
        de.PaintWindows();

        update_palette_effects();
        _uiContext->Draw(dpi);

        if ((gScreenFlags & SCREEN_FLAGS_TITLE_DEMO) && !title_should_hide_version_info())
        {
            DrawOpenRCT2(dpi, 0, _uiContext->GetHeight() - 20);
        }

        gfx_draw_pickedup_peep(dpi);
        gfx_invalidate_pickedup_peep();

        de.PaintRain();
    }

    auto* replayManager = GetContext()->GetReplayManager();
    const char* text = nullptr;

    if (replayManager->IsReplaying())
        text = "Replaying...";
    else if (replayManager->ShouldDisplayNotice())
        text = "Recording...";
    else if (replayManager->IsNormalising())
        text = "Normalising...";

    if (text != nullptr)
        PaintReplayNotice(dpi, text);

    if (gConfigGeneral.show_fps)
    {
        PaintFPS(dpi);
    }
    gCurrentDrawCount++;
}

void Painter::PaintReplayNotice(rct_drawpixelinfo* dpi, const char* text)
{
    int32_t x = _uiContext->GetWidth() / 2;
    int32_t y = _uiContext->GetHeight() - 44;

    // Format string
    utf8 buffer[64] = { 0 };
    utf8* ch = buffer;
    ch = utf8_write_codepoint(ch, FORMAT_MEDIUMFONT);
    ch = utf8_write_codepoint(ch, FORMAT_OUTLINE);
    ch = utf8_write_codepoint(ch, FORMAT_RED);

    snprintf(ch, 64 - (ch - buffer), "%s", text);

    int32_t stringWidth = gfx_get_string_width(buffer);
    x = x - stringWidth;

    if (((gCurrentTicks >> 1) & 0xF) > 4)
        gfx_draw_string(dpi, buffer, COLOUR_SATURATED_RED, x, y);

    // Make area dirty so the text doesn't get drawn over the last
    gfx_set_dirty_blocks(x, y, x + stringWidth, y + 16);
}

void Painter::PaintFPS(rct_drawpixelinfo* dpi)
{
    int32_t x = _uiContext->GetWidth() / 2;
    int32_t y = 2;

    // Measure FPS
    MeasureFPS();

    // Format string
    utf8 buffer[64] = { 0 };
    utf8* ch = buffer;
    ch = utf8_write_codepoint(ch, FORMAT_MEDIUMFONT);
    ch = utf8_write_codepoint(ch, FORMAT_OUTLINE);
    ch = utf8_write_codepoint(ch, FORMAT_WHITE);

    snprintf(ch, 64 - (ch - buffer), "%d", _currentFPS);

    // Draw Text
    int32_t stringWidth = gfx_get_string_width(buffer);
    x = x - (stringWidth / 2);
    gfx_draw_string(dpi, buffer, 0, x, y);

    // Make area dirty so the text doesn't get drawn over the last
    gfx_set_dirty_blocks(x - 16, y - 4, gLastDrawStringX + 16, 16);
}

void Painter::MeasureFPS()
{
    _frames++;

    auto currentTime = time(nullptr);
    if (currentTime != _lastSecond)
    {
        _currentFPS = _frames;
        _frames = 0;
    }
    _lastSecond = currentTime;
}

paint_session* Painter::CreateSession(rct_drawpixelinfo* dpi, uint32_t viewFlags)
{
    paint_session* session = nullptr;

    if (_freePaintSessions.empty() == false)
    {
        // Re-use.
        const size_t idx = _freePaintSessions.size() - 1;
        session = _freePaintSessions[idx];

        // Shrink by one.
        _freePaintSessions.pop_back();
    }
    else
    {
        // Create new one in pool.
        _paintSessionPool.emplace_back(std::make_unique<paint_session>());
        session = _paintSessionPool.back().get();
    }

    session->DPI = *dpi;
    session->EndOfPaintStructArray = &session->PaintStructs[4000 - 1];
    session->NextFreePaintStruct = session->PaintStructs;
    session->LastRootPS = nullptr;
    session->UnkF1AD2C = nullptr;
    session->ViewFlags = viewFlags;
    for (auto& quadrant : session->Quadrants)
    {
        quadrant = nullptr;
    }
    session->QuadrantBackIndex = std::numeric_limits<uint32_t>::max();
    session->QuadrantFrontIndex = 0;
    session->PSStringHead = nullptr;
    session->LastPSString = nullptr;
    session->WoodenSupportsPrependTo = nullptr;
    session->CurrentlyDrawnItem = nullptr;
    session->SurfaceElement = nullptr;

    return session;
}

void Painter::ReleaseSession(paint_session* session)
{
    _freePaintSessions.push_back(session);
}
