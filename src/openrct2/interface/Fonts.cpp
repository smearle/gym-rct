/*****************************************************************************
 * Copyright (c) 2014-2019 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "Fonts.h"

#include "../config/Config.h"
#include "../core/String.hpp"
#include "../drawing/TTF.h"
#include "../localisation/Language.h"
#include "../localisation/LocalisationService.h"
#include "FontFamilies.h"

using namespace OpenRCT2::Localisation;

#ifndef NO_TTF
uint8_t const HINTING_DISABLED = 0;
uint8_t const HINTING_THRESHOLD_LOW = 40;
uint8_t const HINTING_THRESHOLD_MEDIUM = 60;

// clang-format off
TTFFontSetDescriptor TTFFontMSGothic = { {
    { "msgothic.ttc", "MS PGothic",  9, 1,  1,  9, HINTING_THRESHOLD_MEDIUM, nullptr },
    { "msgothic.ttc", "MS PGothic", 12, 1,  0, 14, HINTING_THRESHOLD_MEDIUM, nullptr },
    { "msgothic.ttc", "MS PGothic", 12, 1,  0, 14, HINTING_THRESHOLD_MEDIUM, nullptr },
} };

TTFFontSetDescriptor TTFFontNotoSansMono = { {
    { "NotoSansCJK-Regular.ttc", "Noto Sans Mono CJK JP",  9, 1, -2,  9, HINTING_THRESHOLD_MEDIUM, nullptr },
    { "NotoSansCJK-Regular.ttc", "Noto Sans Mono CJK JP", 12, 1, -3, 14, HINTING_THRESHOLD_MEDIUM, nullptr },
    { "NotoSansCJK-Regular.ttc", "Noto Sans Mono CJK JP", 12, 1, -3, 14, HINTING_THRESHOLD_MEDIUM, nullptr },
} };

TTFFontSetDescriptor TTFFontHiragano = { {
    { u8"ヒラギノ丸ゴ ProN W4.ttc", "Hiragino Maru Gothic ProN",  9, 1,  1,  9, HINTING_THRESHOLD_MEDIUM, nullptr },
    { u8"ヒラギノ丸ゴ ProN W4.ttc", "Hiragino Maru Gothic ProN", 11, 1,  0, 13, HINTING_THRESHOLD_MEDIUM, nullptr },
    { u8"ヒラギノ丸ゴ ProN W4.ttc", "Hiragino Maru Gothic ProN", 11, 1,  0, 13, HINTING_THRESHOLD_MEDIUM, nullptr },
} };

TTFFontSetDescriptor TTFFontMingLiu = { {
    {    "msjh.ttc", "JhengHei",  9, -1, -1,  9, HINTING_THRESHOLD_MEDIUM, nullptr },
    { "mingliu.ttc",  "MingLiU", 11,  1,  1, 14, HINTING_THRESHOLD_MEDIUM, nullptr },
    { "mingliu.ttc",  "MingLiU", 12,  1,  0, 15, HINTING_THRESHOLD_MEDIUM, nullptr },
} };

TTFFontSetDescriptor TTFFontHeiti = { {
    { u8"华文黑体.ttf", "STHeiti",  9, -1, -1,  9, HINTING_THRESHOLD_MEDIUM, nullptr },
    { u8"华文黑体.ttf", "STHeiti", 11,  1,  1, 14, HINTING_THRESHOLD_MEDIUM, nullptr },
    { u8"华文黑体.ttf", "STHeiti", 12,  1,  0, 15, HINTING_THRESHOLD_MEDIUM, nullptr },
} };

TTFFontSetDescriptor TTFFontSimSun = { {
    {   "msyh.ttc",  "Microsoft YaHei",  9, -1, -1,  9, HINTING_THRESHOLD_MEDIUM, nullptr },
    { "simsun.ttc", "SimSun", 11,  1, -1, 14, HINTING_THRESHOLD_MEDIUM, nullptr },
    { "simsun.ttc", "SimSun", 12,  1, -2, 15, HINTING_THRESHOLD_MEDIUM, nullptr },
} };

TTFFontSetDescriptor TTFFontLiHeiPro = { {
    { u8"儷黑 Pro.ttf", "LiHei Pro",  9, 1, -1,  9, HINTING_THRESHOLD_MEDIUM, nullptr },
    { u8"儷黑 Pro.ttf", "LiHei Pro", 11, 1,  0, 14, HINTING_THRESHOLD_MEDIUM, nullptr },
    { u8"儷黑 Pro.ttf", "LiHei Pro", 12, 1,  0, 15, HINTING_THRESHOLD_MEDIUM, nullptr },
} };

TTFFontSetDescriptor TTFFontGulim = { {
    { "gulim.ttc", "Gulim", 10, 1, 0, 10, HINTING_DISABLED,         nullptr },
    { "gulim.ttc", "Gulim", 12, 1, 0, 15, HINTING_THRESHOLD_MEDIUM, nullptr },
    { "gulim.ttc", "Gulim", 12, 1, 0, 15, HINTING_THRESHOLD_MEDIUM, nullptr },
} };

TTFFontSetDescriptor TTFFontNanum = { {
    { "NanumGothic.ttc", "Nanum Gothic", 10, 1, 0, 10, HINTING_DISABLED,      nullptr },
    { "NanumGothic.ttc", "Nanum Gothic", 12, 1, 0, 15, HINTING_THRESHOLD_LOW, nullptr },
    { "NanumGothic.ttc", "Nanum Gothic", 12, 1, 0, 15, HINTING_THRESHOLD_LOW, nullptr },
} };

TTFFontSetDescriptor TTFFontArial = { {
    { "arial.ttf", "Arial",  9, 0, -1,  9, HINTING_THRESHOLD_LOW, nullptr },
    { "arial.ttf", "Arial", 10, 0, -1, 12, HINTING_THRESHOLD_LOW, nullptr },
    { "arial.ttf", "Arial", 11, 0, -1, 12, HINTING_THRESHOLD_LOW, nullptr },
} };

TTFFontSetDescriptor TTFFontArialUnicode = { {
    { "arialuni.ttf", "Arial Unicode MS",  9, 0, -1,  9, HINTING_THRESHOLD_LOW, nullptr },
    { "arialuni.ttf", "Arial Unicode MS", 10, 0, -1, 12, HINTING_THRESHOLD_LOW, nullptr },
    { "arialuni.ttf", "Arial Unicode MS", 11, 0, -1, 12, HINTING_THRESHOLD_LOW, nullptr },
} };

TTFFontSetDescriptor TTFFontMicroHei = { {
    { "wqy-microhei.ttc", "WenQuanYi Micro Hei", 10, -1, -1,  10, HINTING_THRESHOLD_MEDIUM, nullptr },
    { "wqy-microhei.ttc", "WenQuanYi Micro Hei", 11,  1, -1, 14, HINTING_THRESHOLD_MEDIUM, nullptr },
    { "wqy-microhei.ttc", "WenQuanYi Micro Hei", 11,  1, -2, 14, HINTING_THRESHOLD_MEDIUM, nullptr },
} };
// clang-format on
#endif // NO_TTF

static void LoadSpriteFont(LocalisationService& localisationService)
{
    ttf_dispose();
    localisationService.UseTrueTypeFont(false);
#ifndef NO_TTF
    gCurrentTTFFontSet = nullptr;
#endif // NO_TTF
}

#ifndef NO_TTF
static bool LoadFont(LocalisationService& localisationService, TTFFontSetDescriptor* font)
{
    localisationService.UseTrueTypeFont(true);
    gCurrentTTFFontSet = font;

    ttf_dispose();
    bool fontInitialised = ttf_initialise();
    return fontInitialised;
}

static bool LoadCustomConfigFont(LocalisationService& localisationService)
{
    static TTFFontSetDescriptor TTFFontCustom = { {
        { gConfigFonts.file_name, gConfigFonts.font_name, gConfigFonts.size_tiny, gConfigFonts.x_offset, gConfigFonts.y_offset,
          gConfigFonts.height_tiny, gConfigFonts.hinting_threshold, nullptr },
        { gConfigFonts.file_name, gConfigFonts.font_name, gConfigFonts.size_small, gConfigFonts.x_offset, gConfigFonts.y_offset,
          gConfigFonts.height_small, gConfigFonts.hinting_threshold, nullptr },
        { gConfigFonts.file_name, gConfigFonts.font_name, gConfigFonts.size_medium, gConfigFonts.x_offset,
          gConfigFonts.y_offset, gConfigFonts.height_medium, gConfigFonts.hinting_threshold, nullptr },
    } };

    ttf_dispose();
    localisationService.UseTrueTypeFont(true);
    gCurrentTTFFontSet = &TTFFontCustom;

    bool fontInitialised = ttf_initialise();
    return fontInitialised;
}
#endif // NO_TTF

void TryLoadFonts(LocalisationService& localisationService)
{
#ifndef NO_TTF
    auto currentLanguage = localisationService.GetCurrentLanguage();
    TTFontFamily const* fontFamily = LanguagesDescriptors[currentLanguage].font_family;

    if (fontFamily != FAMILY_OPENRCT2_SPRITE)
    {
        if (!String::IsNullOrEmpty(gConfigFonts.file_name))
        {
            if (LoadCustomConfigFont(localisationService))
            {
                return;
            }
            log_verbose("Unable to initialise configured TrueType font -- falling back to the language's default.");
        }

        for (auto& font : *fontFamily)
        {
            if (LoadFont(localisationService, font))
            {
                return;
            }

            TTFFontDescriptor smallFont = font->size[FONT_SIZE_SMALL];
            log_verbose("Unable to load TrueType font '%s' -- trying the next font in the family.", smallFont.font_name);
        }

        if (fontFamily != &TTFFamilySansSerif)
        {
            log_verbose("Unable to initialise any of the preferred TrueType fonts -- falling back to sans serif fonts.");

            for (auto& font : TTFFamilySansSerif)
            {
                if (LoadFont(localisationService, font))
                {
                    return;
                }

                TTFFontDescriptor smallFont = font->size[FONT_SIZE_SMALL];
                log_verbose("Unable to load TrueType font '%s' -- trying the next font in the family.", smallFont.font_name);
            }

            log_verbose("Unable to initialise any of the preferred TrueType fonts -- falling back to sprite font.");
        }
    }
#endif // NO_TTF
    LoadSpriteFont(localisationService);
}
