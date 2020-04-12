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
#include "../ride/RideTypes.h"
#include "Location.hpp"

#include <string>

struct TileElement;
struct WallElement;

using BannerIndex = uint16_t;

constexpr uint8_t BANNER_NULL = 255;
constexpr size_t MAX_BANNERS = 250;
constexpr BannerIndex BANNER_INDEX_NULL = (BannerIndex)-1;

constexpr uint8_t SCROLLING_MODE_NONE = 255;

struct Banner
{
    uint8_t type = BANNER_NULL;
    uint8_t flags{};
    std::string text;
    uint8_t colour{};
    ride_id_t ride_index{};
    uint8_t text_colour{};
    TileCoordsXY position;

    bool IsNull() const
    {
        return type == BANNER_NULL;
    }

    std::string GetText() const;
    size_t FormatTextTo(void* args) const;
};

enum BANNER_FLAGS
{
    BANNER_FLAG_NO_ENTRY = (1 << 0),
    BANNER_FLAG_IS_LARGE_SCENERY = (1 << 1),
    BANNER_FLAG_LINKED_TO_RIDE = (1 << 2),
    BANNER_FLAG_IS_WALL = (1 << 3)
};

void banner_init();
BannerIndex create_new_banner(uint8_t flags);
TileElement* banner_get_tile_element(BannerIndex bannerIndex);
WallElement* banner_get_scrolling_wall_tile_element(BannerIndex bannerIndex);
uint8_t banner_get_closest_ride_index(const CoordsXYZ& mapPos);
void banner_reset_broken_index();
void fix_duplicated_banners();
Banner* GetBanner(BannerIndex id);
