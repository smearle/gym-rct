/*****************************************************************************
 * Copyright (c) 2014-2019 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include "../Cheats.h"
#include "../OpenRCT2.h"
#include "../core/MemoryStream.h"
#include "../interface/Window.h"
#include "../localisation/StringIds.h"
#include "../management/Finance.h"
#include "../world/Footpath.h"
#include "../world/Location.hpp"
#include "../world/Park.h"
#include "../world/Wall.h"
#include "BannerRemoveAction.hpp"
#include "GameAction.h"

DEFINE_GAME_ACTION(FootpathRemoveAction, GAME_COMMAND_REMOVE_PATH, GameActionResult)
{
private:
    CoordsXYZ _loc;

public:
    FootpathRemoveAction() = default;
    FootpathRemoveAction(const CoordsXYZ& location)
        : _loc(location)
    {
    }

    uint16_t GetActionFlags() const override
    {
        return GameAction::GetActionFlags();
    }

    void Serialise(DataSerialiser & stream) override
    {
        GameAction::Serialise(stream);

        stream << DS_TAG(_loc);
    }

    GameActionResult::Ptr Query() const override
    {
        GameActionResult::Ptr res = std::make_unique<GameActionResult>();
        res->Cost = 0;
        res->Expenditure = ExpenditureType::Landscaping;
        res->Position = { _loc.x + 16, _loc.y + 16, _loc.z };

        if (!((gScreenFlags & SCREEN_FLAGS_SCENARIO_EDITOR) || gCheatsSandboxMode) && !map_is_location_owned(_loc))
        {
            return MakeResult(GA_ERROR::NOT_OWNED, STR_CANT_REMOVE_FOOTPATH_FROM_HERE, STR_LAND_NOT_OWNED_BY_PARK);
        }

        TileElement* footpathElement = GetFootpathElement();
        if (footpathElement == nullptr)
        {
            return MakeResult(GA_ERROR::INVALID_PARAMETERS, STR_CANT_REMOVE_FOOTPATH_FROM_HERE);
        }

        res->Cost = GetRefundPrice(footpathElement);

        return res;
    }

    GameActionResult::Ptr Execute() const override
    {
        GameActionResult::Ptr res = std::make_unique<GameActionResult>();
        res->Cost = 0;
        res->Expenditure = ExpenditureType::Landscaping;
        res->Position = { _loc.x + 16, _loc.y + 16, _loc.z };

        if (!(GetFlags() & GAME_COMMAND_FLAG_GHOST))
        {
            footpath_interrupt_peeps(_loc);
            footpath_remove_litter(_loc);
        }

        TileElement* footpathElement = GetFootpathElement();
        if (footpathElement != nullptr)
        {
            footpath_queue_chain_reset();
            auto bannerRes = RemoveBannersAtElement(_loc, footpathElement);
            if (bannerRes->Error == GA_ERROR::OK)
            {
                res->Cost += bannerRes->Cost;
            }
            footpath_remove_edges_at(_loc, footpathElement);
            map_invalidate_tile_full(_loc);
            tile_element_remove(footpathElement);
            footpath_update_queue_chains();
        }
        else
        {
            return MakeResult(GA_ERROR::INVALID_PARAMETERS, STR_CANT_REMOVE_FOOTPATH_FROM_HERE);
        }

        res->Cost += GetRefundPrice(footpathElement);

        return res;
    }

private:
    TileElement* GetFootpathElement() const
    {
        bool getGhostPath = GetFlags() & GAME_COMMAND_FLAG_GHOST;

        TileElement* tileElement = map_get_footpath_element(_loc);
        TileElement* footpathElement = nullptr;
        if (tileElement != nullptr)
        {
            if (getGhostPath && !tileElement->IsGhost())
            {
                while (!(tileElement++)->IsLastForTile())
                {
                    if (tileElement->GetType() != TILE_ELEMENT_TYPE_PATH && !tileElement->IsGhost())
                    {
                        continue;
                    }
                    footpathElement = tileElement;
                    break;
                }
            }
            else
            {
                footpathElement = tileElement;
            }
        }

        return footpathElement;
    }

    money32 GetRefundPrice(TileElement * footpathElement) const
    {
        money32 cost = -MONEY(10, 00);
        return cost;
    }

    /**
     *
     *  rct2: 0x006BA23E
     */
    GameActionResult::Ptr RemoveBannersAtElement(const CoordsXY& loc, TileElement* tileElement) const
    {
        auto result = MakeResult();
        while (!(tileElement++)->IsLastForTile())
        {
            if (tileElement->GetType() == TILE_ELEMENT_TYPE_PATH)
                return result;
            else if (tileElement->GetType() != TILE_ELEMENT_TYPE_BANNER)
                continue;

            auto bannerRemoveAction = BannerRemoveAction(
                { loc, tileElement->GetBaseZ(), tileElement->AsBanner()->GetPosition() });
            bool isGhost = tileElement->IsGhost();
            auto bannerFlags = GetFlags() | (isGhost ? static_cast<uint32_t>(GAME_COMMAND_FLAG_GHOST) : 0);
            bannerRemoveAction.SetFlags(bannerFlags);
            auto res = GameActions::ExecuteNested(&bannerRemoveAction);
            // Ghost removal is free
            if (res->Error == GA_ERROR::OK && !isGhost)
            {
                result->Cost += res->Cost;
            }
            tileElement--;
        }
        return result;
    }
};
