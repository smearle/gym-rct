/*****************************************************************************
 * Copyright (c) 2014-2018 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "Addresses.h"

#include <openrct2/config/Config.h>
#include <openrct2/interface/Colour.h>
#include <openrct2/interface/Viewport.h>
#include <openrct2/object/Object.h>
#include <openrct2/paint/tile_element/Paint.TileElement.h>
#include <openrct2/ride/Ride.h>
#include <openrct2/ride/Track.h>
#include <openrct2/world/Location.hpp>
#include <openrct2/world/Sprite.h>
#include <openrct2/world/Surface.h>

class StationObject;

#define gRideEntries RCT2_ADDRESS(0x009ACFA4, rct_ride_entry*)
#define gTileElementTilePointers RCT2_ADDRESS(0x013CE9A4, TileElement*)
rct_sprite* sprite_list = RCT2_ADDRESS(0x010E63BC, rct_sprite);

Ride gRideList[MAX_RIDES];
int16_t gMapSizeUnits;
int16_t gMapBaseZ;
bool gTrackDesignSaveMode = false;
uint8_t gTrackDesignSaveRideIndex = RIDE_ID_NULL;
uint8_t gClipHeight = 255;
TileCoordsXY gClipSelectionA = { 0, 0 };
TileCoordsXY gClipSelectionB = { MAXIMUM_MAP_SIZE_TECHNICAL - 1, MAXIMUM_MAP_SIZE_TECHNICAL - 1 };
uint32_t gScenarioTicks;
uint8_t gCurrentRotation;

// clang-format off
const CoordsXY CoordsDirectionDelta[] = {
    { -32, 0 },
    { 0, +32 },
    { +32, 0 },
    { 0, -32 },
    { -32, +32 },
    { +32, +32 },
    { +32, -32 },
    { -32, -32 },
};

const TileCoordsXY TileDirectionDelta[] = {
    { -1, 0 },
    { 0, +1 },
    { +1, 0 },
    { 0, -1 },
    { -1, +1 },
    { +1, +1 },
    { +1, -1 },
    { -1, -1 },
};
// clang-format on

TileCoordsXYZD ride_get_entrance_location(const Ride* ride, const int32_t stationIndex);
TileCoordsXYZD ride_get_exit_location(const Ride* ride, const int32_t stationIndex);

uint8_t get_current_rotation()
{
    return gCurrentRotation & 3;
}

const uint32_t construction_markers[] = {
    COLOUR_DARK_GREEN << 19 | COLOUR_GREY << 24 | IMAGE_TYPE_REMAP, // White
    2 << 19 | 0b110000 << 19 | IMAGE_TYPE_TRANSPARENT,              // Translucent
};

int object_entry_group_counts[] = {
    128, // rides
    252, // small scenery
    128, // large scenery
    128, // walls
    32,  // banners
    16,  // paths
    15,  // path bits
    19,  // scenery sets
    1,   // park entrance
    1,   // water
    1    // scenario text
};

GeneralConfiguration gConfigGeneral;
uint16_t gMapSelectFlags;
uint16_t gMapSelectType;
CoordsXY gMapSelectPositionA;
CoordsXY gMapSelectPositionB;
CoordsXYZ gMapSelectArrowPosition;
uint8_t gMapSelectArrowDirection;

void entrance_paint(paint_session* session, uint8_t direction, int height, const TileElement* tile_element)
{
}
void banner_paint(paint_session* session, uint8_t direction, int height, const TileElement* tile_element)
{
}
void surface_paint(paint_session* session, uint8_t direction, uint16_t height, const TileElement* tileElement)
{
}
void path_paint(paint_session* session, uint16_t height, const TileElement* tileElement)
{
}
void scenery_paint(paint_session* session, uint8_t direction, int height, const TileElement* tileElement)
{
}
void fence_paint(paint_session* session, uint8_t direction, int height, const TileElement* tileElement)
{
}
void large_scenery_paint(paint_session* session, uint8_t direction, uint16_t height, const TileElement* tileElement)
{
}

Ride* get_ride(ride_id_t index)
{
    if (index >= RCT12_MAX_RIDES_IN_PARK)
    {
        log_error("invalid index %d for ride", index);
        return nullptr;
    }
    return &gRideList[index];
}

rct_ride_entry* get_ride_entry(int index)
{
    if (index < 0 || index >= object_entry_group_counts[OBJECT_TYPE_RIDE])
    {
        log_error("invalid index %d for ride type", index);
        return nullptr;
    }

    return gRideEntries[index];
}

rct_ride_entry* Ride::GetRideEntry() const
{
    rct_ride_entry* rideEntry = get_ride_entry(subtype);
    if (rideEntry == nullptr)
    {
        log_error("Invalid ride subtype for ride");
    }
    return rideEntry;
}

rct_sprite* get_sprite(size_t sprite_idx)
{
    assert(sprite_idx < MAX_SPRITES);
    return &sprite_list[sprite_idx];
}

bool TileElementBase::IsLastForTile() const
{
    return (this->flags & TILE_ELEMENT_FLAG_LAST_TILE) != 0;
}

void TileElementBase::SetLastForTile(bool on)
{
    if (on)
        flags |= TILE_ELEMENT_FLAG_LAST_TILE;
    else
        flags &= ~TILE_ELEMENT_FLAG_LAST_TILE;
}

uint8_t TileElementBase::GetType() const
{
    return this->type & TILE_ELEMENT_TYPE_MASK;
}

bool TileElementBase::IsGhost() const
{
    return (this->flags & TILE_ELEMENT_FLAG_GHOST) != 0;
}

bool TrackElement::BlockBrakeClosed() const
{
    return (flags & TILE_ELEMENT_FLAG_BLOCK_BRAKE_CLOSED) != 0;
}

TileElement* map_get_first_element_at(const CoordsXY& elementPos)
{
    if (elementPos.x < 0 || elementPos.y < 0 || elementPos.x > 255 || elementPos.y > 255)
    {
        log_error("Trying to access element outside of range");
        return nullptr;
    }
    auto tileElementPos = TileCoordsXY{ elementPos };
    return gTileElementTilePointers[tileElementPos.x + tileElementPos.y * 256];
}

bool ride_type_has_flag(int rideType, uint32_t flag)
{
    return (RideProperties[rideType].flags & flag) != 0;
}

int16_t get_height_marker_offset()
{
    return 0;
}

bool is_csg_loaded()
{
    return false;
}

uint8_t TrackElement::GetSeatRotation() const
{
    return ColourScheme >> 4;
}

void TrackElement::SetSeatRotation(uint8_t newSeatRotation)
{
    ColourScheme &= ~TRACK_ELEMENT_COLOUR_SEAT_ROTATION_MASK;
    ColourScheme |= (newSeatRotation << 4);
}

bool TrackElement::IsTakingPhoto() const
{
    return OnridePhotoBits != 0;
}

void TrackElement::SetPhotoTimeout()
{
    OnridePhotoBits = 3;
}

void TrackElement::SetPhotoTimeout(uint8_t value)
{
    OnridePhotoBits = value;
}

uint8_t TrackElement::GetPhotoTimeout() const
{
    return OnridePhotoBits;
}

void TrackElement::DecrementPhotoTimeout()
{
    OnridePhotoBits = std::max(0, OnridePhotoBits - 1);
}

uint16_t TrackElement::GetMazeEntry() const
{
    return MazeEntry;
}

void TrackElement::SetMazeEntry(uint16_t newMazeEntry)
{
    MazeEntry = newMazeEntry;
}

void TrackElement::MazeEntryAdd(uint16_t addVal)
{
    MazeEntry |= addVal;
}

void TrackElement::MazeEntrySubtract(uint16_t subVal)
{
    MazeEntry &= ~subVal;
}

track_type_t TrackElement::GetTrackType() const
{
    return TrackType;
}

void TrackElement::SetTrackType(track_type_t newType)
{
    TrackType = newType;
}

uint8_t TrackElement::GetSequenceIndex() const
{
    return Sequence;
}

void TrackElement::SetSequenceIndex(uint8_t newSequenceIndex)
{
    Sequence = newSequenceIndex;
}

uint8_t TrackElement::GetStationIndex() const
{
    return StationIndex;
}

void TrackElement::SetStationIndex(uint8_t newStationIndex)
{
    StationIndex = newStationIndex;
}

uint8_t TrackElement::GetDoorAState() const
{
    return (ColourScheme & TRACK_ELEMENT_COLOUR_DOOR_A_MASK) >> 2;
}

uint8_t TrackElement::GetDoorBState() const
{
    return (ColourScheme & TRACK_ELEMENT_COLOUR_DOOR_B_MASK) >> 5;
}

ride_idnew_t TrackElement::GetRideIndex() const
{
    return RideIndex;
}

void TrackElement::SetRideIndex(ride_idnew_t newRideIndex)
{
    RideIndex = newRideIndex;
}

uint8_t TrackElement::GetColourScheme() const
{
    return ColourScheme & TRACK_ELEMENT_COLOUR_SCHEME_MASK;
}

void TrackElement::SetColourScheme(uint8_t newColourScheme)
{
    ColourScheme &= ~TRACK_ELEMENT_COLOUR_SCHEME_MASK;
    ColourScheme |= (newColourScheme & TRACK_ELEMENT_COLOUR_SCHEME_MASK);
}

bool TrackElement::HasCableLift() const
{
    return Flags2 & TRACK_ELEMENT_FLAGS2_CABLE_LIFT;
}

void TrackElement::SetHasCableLift(bool on)
{
    Flags2 &= ~TRACK_ELEMENT_FLAGS2_CABLE_LIFT;
    if (on)
        Flags2 |= TRACK_ELEMENT_FLAGS2_CABLE_LIFT;
}

bool TrackElement::IsInverted() const
{
    return Flags2 & TRACK_ELEMENT_FLAGS2_INVERTED;
}

void TrackElement::SetInverted(bool inverted)
{
    if (inverted)
    {
        Flags2 |= TRACK_ELEMENT_FLAGS2_INVERTED;
    }
    else
    {
        Flags2 &= ~TRACK_ELEMENT_FLAGS2_INVERTED;
    }
}

uint8_t TrackElement::GetBrakeBoosterSpeed() const
{
    return BrakeBoosterSpeed << 1;
}

void TrackElement::SetBrakeBoosterSpeed(uint8_t speed)
{
    BrakeBoosterSpeed = (speed >> 1);
}

bool TrackElement::HasGreenLight() const
{
    return (Flags2 & TRACK_ELEMENT_FLAGS2_HAS_GREEN_LIGHT) != 0;
}

void TrackElement::SetHasGreenLight(bool on)
{
    Flags2 &= ~TRACK_ELEMENT_FLAGS2_HAS_GREEN_LIGHT;
    if (on)
    {
        Flags2 |= TRACK_ELEMENT_FLAGS2_HAS_GREEN_LIGHT;
    }
}

bool TrackElement::HasChain() const
{
    return (Flags2 & TRACK_ELEMENT_FLAGS2_CHAIN_LIFT) != 0;
}

void TrackElement::SetHasChain(bool on)
{
    if (on)
    {
        Flags2 |= TRACK_ELEMENT_FLAGS2_CHAIN_LIFT;
    }
    else
    {
        Flags2 &= ~TRACK_ELEMENT_FLAGS2_CHAIN_LIFT;
    }
}

TileCoordsXYZD ride_get_entrance_location(const Ride* ride, const int32_t stationIndex)
{
    return ride->stations[stationIndex].Entrance;
}

TileCoordsXYZD ride_get_exit_location(const Ride* ride, const int32_t stationIndex)
{
    return ride->stations[stationIndex].Exit;
}

void TileElementBase::SetType(uint8_t newType)
{
    this->type &= ~TILE_ELEMENT_TYPE_MASK;
    this->type |= (newType & TILE_ELEMENT_TYPE_MASK);
}

uint8_t TileElementBase::GetDirection() const
{
    return this->type & TILE_ELEMENT_DIRECTION_MASK;
}

uint8_t TileElementBase::GetDirectionWithOffset(uint8_t offset) const
{
    return ((this->type & TILE_ELEMENT_DIRECTION_MASK) + offset) & TILE_ELEMENT_DIRECTION_MASK;
}

uint8_t SurfaceElement::GetSlope() const
{
    return Slope;
}

uint32_t SurfaceElement::GetWaterHeight() const
{
    return WaterHeight;
}

bool TrackElement::IsHighlighted() const
{
    return (Flags2 & TRACK_ELEMENT_FLAGS2_HIGHLIGHT);
}

uint8_t PathElement::GetEdges() const
{
    return edges & 0xF;
}

StationObject* ride_get_station_object(const Ride* ride)
{
    return nullptr;
}

bool rct_vehicle::IsGhost() const
{
    auto r = get_ride(ride);
    return r != nullptr && r->status == RIDE_STATUS_SIMULATING;
}

uint8_t TileElementBase::GetOccupiedQuadrants() const
{
    return flags & TILE_ELEMENT_OCCUPIED_QUADRANTS_MASK;
}

void TileElementBase::SetOccupiedQuadrants(uint8_t quadrants)
{
    flags &= ~TILE_ELEMENT_OCCUPIED_QUADRANTS_MASK;
    flags |= (quadrants & TILE_ELEMENT_OCCUPIED_QUADRANTS_MASK);
}

int32_t TileElementBase::GetBaseZ() const
{
    return base_height * 8;
}

void TileElementBase::SetBaseZ(int32_t newZ)
{
    base_height = (newZ / 8);
}

int32_t TileElementBase::GetClearanceZ() const
{
    return clearance_height * 8;
}

void TileElementBase::SetClearanceZ(int32_t newZ)
{
    clearance_height = (newZ / 8);
}

int32_t RideStation::GetBaseZ() const
{
    return Height * 8;
}

void RideStation::SetBaseZ(int32_t newZ)
{
    Height = newZ / 8;
}

CoordsXYZ RideStation::GetStart() const
{
    TileCoordsXYZ stationTileCoords{ Start.x, Start.y, Height };
    return stationTileCoords.ToCoordsXYZ();
}
