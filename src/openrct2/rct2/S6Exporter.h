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
#include "../object/ObjectList.h"
#include "../scenario/Scenario.h"

#include <optional>
#include <string>
#include <string_view>
#include <vector>

interface IStream;
struct ObjectRepositoryItem;
struct RCT12SpriteBase;
struct SpriteBase;

/**
 * Class to export RollerCoaster Tycoon 2 scenarios (*.SC6) and saved games (*.SV6).
 */
class S6Exporter final
{
public:
    bool RemoveTracklessRides;
    std::vector<const ObjectRepositoryItem*> ExportObjectsList;

    S6Exporter();

    void SaveGame(const utf8* path);
    void SaveGame(IStream* stream);
    void SaveScenario(const utf8* path);
    void SaveScenario(IStream* stream);
    void Export();
    void ExportParkName();
    void ExportRides();
    void ExportRide(rct2_ride* dst, const Ride* src);
    void ExportSprites();
    void ExportSprite(RCT2Sprite* dst, const rct_sprite* src);
    void ExportSpriteCommonProperties(RCT12SpriteBase* dst, const SpriteBase* src);
    void ExportSpriteVehicle(RCT2SpriteVehicle* dst, const Vehicle* src);
    void ExportSpritePeep(RCT2SpritePeep* dst, const Peep* src);
    void ExportSpriteMisc(RCT12SpriteBase* dst, const SpriteBase* src);
    void ExportSpriteLitter(RCT12SpriteLitter* dst, const Litter* src);

private:
    rct_s6_data _s6{};
    std::vector<std::string> _userStrings;

    void Save(IStream* stream, bool isScenario);
    static uint32_t GetLoanHash(money32 initialCash, money32 bankLoan, uint32_t maxBankLoan);
    void ExportResearchedRideTypes();
    void ExportResearchedRideEntries();
    void ExportResearchedSceneryItems();
    void ExportResearchList();
    void ExportMarketingCampaigns();
    void ExportPeepSpawns();
    void ExportRideRatingsCalcData();
    void ExportRideMeasurements();
    void ExportRideMeasurement(RCT12RideMeasurement& dst, const RideMeasurement& src);
    void ExportBanners();
    void ExportBanner(RCT12Banner& dst, const Banner& src);
    void ExportMapAnimations();

    void ExportTileElements();
    void ExportTileElement(RCT12TileElement* dst, TileElement* src);

    std::optional<uint16_t> AllocateUserString(const std::string_view& value);
    void ExportUserStrings();
};
