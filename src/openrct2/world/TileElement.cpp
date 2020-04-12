/*****************************************************************************
 * Copyright (c) 2014-2019 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "TileElement.h"

#include "../core/Guard.hpp"
#include "../interface/Window.h"
#include "../localisation/Localisation.h"
#include "../ride/Track.h"
#include "Banner.h"
#include "LargeScenery.h"
#include "Scenery.h"

uint8_t TileElementBase::GetType() const
{
    return this->type & TILE_ELEMENT_TYPE_MASK;
}

void TileElementBase::SetType(uint8_t newType)
{
    this->type &= ~TILE_ELEMENT_TYPE_MASK;
    this->type |= (newType & TILE_ELEMENT_TYPE_MASK);
}

Direction TileElementBase::GetDirection() const
{
    return this->type & TILE_ELEMENT_DIRECTION_MASK;
}

void TileElementBase::SetDirection(Direction direction)
{
    this->type &= ~TILE_ELEMENT_DIRECTION_MASK;
    this->type |= (direction & TILE_ELEMENT_DIRECTION_MASK);
}

Direction TileElementBase::GetDirectionWithOffset(uint8_t offset) const
{
    return ((this->type & TILE_ELEMENT_DIRECTION_MASK) + offset) & TILE_ELEMENT_DIRECTION_MASK;
}

bool TileElementBase::IsLastForTile() const
{
    return (this->Flags & TILE_ELEMENT_FLAG_LAST_TILE) != 0;
}

void TileElementBase::SetLastForTile(bool on)
{
    if (on)
        Flags |= TILE_ELEMENT_FLAG_LAST_TILE;
    else
        Flags &= ~TILE_ELEMENT_FLAG_LAST_TILE;
}

bool TileElementBase::IsGhost() const
{
    return (this->Flags & TILE_ELEMENT_FLAG_GHOST) != 0;
}

void TileElementBase::SetGhost(bool isGhost)
{
    if (isGhost)
    {
        this->Flags |= TILE_ELEMENT_FLAG_GHOST;
    }
    else
    {
        this->Flags &= ~TILE_ELEMENT_FLAG_GHOST;
    }
}

bool tile_element_is_underground(TileElement* tileElement)
{
    do
    {
        tileElement++;
        if ((tileElement - 1)->IsLastForTile())
            return false;
    } while (tileElement->GetType() != TILE_ELEMENT_TYPE_SURFACE);
    return true;
}

BannerIndex tile_element_get_banner_index(TileElement* tileElement)
{
    rct_scenery_entry* sceneryEntry;

    switch (tileElement->GetType())
    {
        case TILE_ELEMENT_TYPE_LARGE_SCENERY:
            sceneryEntry = tileElement->AsLargeScenery()->GetEntry();
            if (sceneryEntry->large_scenery.scrolling_mode == SCROLLING_MODE_NONE)
                return BANNER_INDEX_NULL;

            return tileElement->AsLargeScenery()->GetBannerIndex();
        case TILE_ELEMENT_TYPE_WALL:
            sceneryEntry = tileElement->AsWall()->GetEntry();
            if (sceneryEntry == nullptr || sceneryEntry->wall.scrolling_mode == SCROLLING_MODE_NONE)
                return BANNER_INDEX_NULL;

            return tileElement->AsWall()->GetBannerIndex();
        case TILE_ELEMENT_TYPE_BANNER:
            return tileElement->AsBanner()->GetIndex();
        default:
            return BANNER_INDEX_NULL;
    }
}

void tile_element_set_banner_index(TileElement* tileElement, BannerIndex bannerIndex)
{
    switch (tileElement->GetType())
    {
        case TILE_ELEMENT_TYPE_WALL:
            tileElement->AsWall()->SetBannerIndex(bannerIndex);
            break;
        case TILE_ELEMENT_TYPE_LARGE_SCENERY:
            tileElement->AsLargeScenery()->SetBannerIndex(bannerIndex);
            break;
        case TILE_ELEMENT_TYPE_BANNER:
            tileElement->AsBanner()->SetIndex(bannerIndex);
            break;
        default:
            log_error("Tried to set banner index on unsuitable tile element!");
            Guard::Assert(false);
    }
}

void tile_element_remove_banner_entry(TileElement* tileElement)
{
    auto bannerIndex = tile_element_get_banner_index(tileElement);
    auto banner = GetBanner(bannerIndex);
    if (banner != nullptr)
    {
        window_close_by_number(WC_BANNER, bannerIndex);
        *banner = {};
    }
}

uint8_t tile_element_get_ride_index(const TileElement* tileElement)
{
    switch (tileElement->GetType())
    {
        case TILE_ELEMENT_TYPE_TRACK:
            return tileElement->AsTrack()->GetRideIndex();
        case TILE_ELEMENT_TYPE_ENTRANCE:
            return tileElement->AsEntrance()->GetRideIndex();
        case TILE_ELEMENT_TYPE_PATH:
            return tileElement->AsPath()->GetRideIndex();
        default:
            return RIDE_ID_NULL;
    }
}

void TileElement::ClearAs(uint8_t newType)
{
    type = newType;
    Flags = 0;
    base_height = MINIMUM_LAND_HEIGHT;
    clearance_height = MINIMUM_LAND_HEIGHT;
    std::fill_n(pad_04, sizeof(pad_04), 0x00);
    std::fill_n(pad_08, sizeof(pad_08), 0x00);
}

void TileElementBase::Remove()
{
    tile_element_remove((TileElement*)this);
}

// Rotate both of the values amount
const QuarterTile QuarterTile::Rotate(uint8_t amount) const
{
    switch (amount)
    {
        case 0:
            return QuarterTile{ *this };
            break;
        case 1:
        {
            auto rotVal1 = _val << 1;
            auto rotVal2 = rotVal1 >> 4;
            // Clear the bit from the tileQuarter
            rotVal1 &= 0b11101110;
            // Clear the bit from the zQuarter
            rotVal2 &= 0b00010001;
            return QuarterTile{ static_cast<uint8_t>(rotVal1 | rotVal2) };
        }
        case 2:
        {
            auto rotVal1 = _val << 2;
            auto rotVal2 = rotVal1 >> 4;
            // Clear the bit from the tileQuarter
            rotVal1 &= 0b11001100;
            // Clear the bit from the zQuarter
            rotVal2 &= 0b00110011;
            return QuarterTile{ static_cast<uint8_t>(rotVal1 | rotVal2) };
        }
        case 3:
        {
            auto rotVal1 = _val << 3;
            auto rotVal2 = rotVal1 >> 4;
            // Clear the bit from the tileQuarter
            rotVal1 &= 0b10001000;
            // Clear the bit from the zQuarter
            rotVal2 &= 0b01110111;
            return QuarterTile{ static_cast<uint8_t>(rotVal1 | rotVal2) };
        }
        default:
            log_error("Tried to rotate QuarterTile invalid amount.");
            return QuarterTile{ 0 };
    }
}

uint8_t TileElementBase::GetOccupiedQuadrants() const
{
    return Flags & TILE_ELEMENT_OCCUPIED_QUADRANTS_MASK;
}

void TileElementBase::SetOccupiedQuadrants(uint8_t quadrants)
{
    Flags &= ~TILE_ELEMENT_OCCUPIED_QUADRANTS_MASK;
    Flags |= (quadrants & TILE_ELEMENT_OCCUPIED_QUADRANTS_MASK);
}

int32_t TileElementBase::GetBaseZ() const
{
    return base_height * COORDS_Z_STEP;
}

void TileElementBase::SetBaseZ(int32_t newZ)
{
    base_height = (newZ / COORDS_Z_STEP);
}

int32_t TileElementBase::GetClearanceZ() const
{
    return clearance_height * COORDS_Z_STEP;
}

void TileElementBase::SetClearanceZ(int32_t newZ)
{
    clearance_height = (newZ / COORDS_Z_STEP);
}
