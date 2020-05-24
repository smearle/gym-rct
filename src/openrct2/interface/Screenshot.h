/*****************************************************************************
 * Copyright (c) 2014-2019 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include "../common.h"

#include <string>
#include "Viewport.h"
#include "../core/Imaging.h"
#include "../drawing/IDrawingEngine.h"

using namespace OpenRCT2::Drawing;

struct rct_drawpixelinfo;

extern uint8_t gScreenshotCountdown;

struct ScreenshotOptions
{
    int32_t weather = 0;
    bool hide_guests = false;
    bool hide_sprites = false;
    bool clear_grass = false;
    bool mowed_grass = false;
    bool water_plants = false;
    bool fix_vandalism = false;
    bool remove_litter = false;
    bool tidy_up_park = false;
    bool transparent = false;
};

void screenshot_check();
std::string screenshot_dump();
std::string screenshot_dump_png(rct_drawpixelinfo* dpi);
std::string screenshot_dump_png_32bpp(int32_t width, int32_t height, const void* pixels);

Image get_observation();

void screenshot_giant();
int32_t cmdline_for_screenshot(const char** argv, int32_t argc, ScreenshotOptions* options);
int32_t cmdline_for_gfxbench(const char** argv, int32_t argc);
