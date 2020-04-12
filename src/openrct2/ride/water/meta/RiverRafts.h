/*****************************************************************************
 * Copyright (c) 2014-2020 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include "../../RideData.h"
#include "../../Track.h"

// clang-format off
constexpr const RideTypeDescriptor RiverRaftsRTD =
{
    SET_FIELD(EnabledTrackPieces, (1ULL << TRACK_STRAIGHT) | (1ULL << TRACK_STATION_END) | (1ULL << TRACK_S_BEND) | (1ULL << TRACK_CURVE)),
    SET_FIELD(ExtraTrackPieces, (1ULL << TRACK_ON_RIDE_PHOTO)),
    SET_FIELD(TrackPaintFunction, get_track_paint_function_splash_boats),
};
// clang-format on
