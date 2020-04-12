/*****************************************************************************
 * Copyright (c) 2014-2019 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#ifndef OPENRCT2_FONTS_H
#define OPENRCT2_FONTS_H

namespace OpenRCT2::Localisation
{
    class LocalisationService;
}

#include "../drawing/Font.h"

#ifndef NO_TTF
extern TTFFontSetDescriptor TTFFontMSGothic;
extern TTFFontSetDescriptor TTFFontNotoSansMono;
extern TTFFontSetDescriptor TTFFontHiragano;
extern TTFFontSetDescriptor TTFFontMingLiu;
extern TTFFontSetDescriptor TTFFontHeiti;
extern TTFFontSetDescriptor TTFFontSimSun;
extern TTFFontSetDescriptor TTFFontLiHeiPro;
extern TTFFontSetDescriptor TTFFontGulim;
extern TTFFontSetDescriptor TTFFontNanum;
extern TTFFontSetDescriptor TTFFontArial;
extern TTFFontSetDescriptor TTFFontArialUnicode;
extern TTFFontSetDescriptor TTFFontMicroHei;
#    define FONT(x) x
#else
#    define FONT(x) FONT_OPENRCT2_SPRITE
#endif // NO_TTF

void TryLoadFonts(OpenRCT2::Localisation::LocalisationService& localisationService);

#endif // OPENRCT2_FONTS_H
