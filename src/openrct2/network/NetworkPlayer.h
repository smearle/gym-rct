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
#include "../peep/Peep.h"
#include "../world/Map.h"
#include "../world/Sprite.h"

#include <string>
#include <unordered_map>

class NetworkPacket;

class NetworkPlayer final
{
public:
    uint8_t Id = 0;
    std::string Name;
    uint16_t Ping = 0;
    uint8_t Flags = 0;
    uint8_t Group = 0;
    money32 MoneySpent = MONEY(0, 0);
    uint32_t CommandsRan = 0;
    int32_t LastAction = -999;
    uint32_t LastActionTime = 0;
    CoordsXYZ LastActionCoord = {};
    Peep* PickupPeep = nullptr;
    int32_t PickupPeepOldX = LOCATION_NULL;
    std::string KeyHash;
    uint32_t LastDemolishRideTime = 0;
    uint32_t LastPlaceSceneryTime = 0;
    std::unordered_map<uint32_t, int32_t> CooldownTime;
    NetworkPlayer() = default;

    void SetName(const std::string& name);

    void Read(NetworkPacket& packet);
    void Write(NetworkPacket& packet);
    void AddMoneySpent(money32 cost);
};
