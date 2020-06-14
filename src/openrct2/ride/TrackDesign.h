/*****************************************************************************
 * Copyright (c) 2014-2019 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#ifndef _TRACK_DESIGN_H_
#define _TRACK_DESIGN_H_

#include "../common.h"
#include "../object/Object.h"
#include "../rct12/RCT12.h"
#include "../rct2/RCT2.h"
#include "../world/Map.h"
#include "Vehicle.h"

struct Ride;

#define TRACK_PREVIEW_IMAGE_SIZE (370 * 217)

/* Track Entrance entry */
struct TrackDesignEntranceElement
{
    int8_t z;
    uint8_t direction;
    int16_t x;
    int16_t y;
    bool isExit;
};

/* Track Scenery entry  size: 0x16 */
struct TrackDesignSceneryElement
{
    rct_object_entry scenery_object; // 0x00
    int8_t x;                        // 0x10
    int8_t y;                        // 0x11
    int8_t z;                        // 0x12
    uint8_t flags;                   // 0x13 direction quadrant tertiary colour
    uint8_t primary_colour;          // 0x14
    uint8_t secondary_colour;        // 0x15
};

/**
 * Track design structure.
 */

/* Track Element entry  size: 0x03 */
struct TrackDesignTrackElement
{
    track_type_t type; // 0x00
    uint8_t flags;     // 0x02
};

/* Maze Element entry   size: 0x04 */
struct TrackDesignMazeElement
{
    union
    {
        uint32_t all;
        struct
        {
            int8_t x;
            int8_t y;
            union
            {
                uint16_t maze_entry;
                struct
                {
                    uint8_t direction;
                    uint8_t type;
                };
            };
        };
    };
};

class DataSerialiser;

struct TrackDesign
{
    uint8_t type;
    uint8_t vehicle_type;
    money32 cost;
    uint32_t flags;
    uint8_t ride_mode;
    uint8_t track_flags;
    uint8_t colour_scheme;
    std::array<rct_vehicle_colour, RCT2_MAX_CARS_PER_TRAIN> vehicle_colours;
    uint8_t entrance_style;
    uint8_t total_air_time;
    uint8_t depart_flags;
    uint8_t number_of_trains;
    uint8_t number_of_cars_per_train;
    uint8_t min_waiting_time;
    uint8_t max_waiting_time;
    uint8_t operation_setting;
    int8_t max_speed;
    int8_t average_speed;
    uint16_t ride_length;
    uint8_t max_positive_vertical_g;
    int8_t max_negative_vertical_g;
    uint8_t max_lateral_g;
    uint8_t inversions;
    uint8_t holes;
    uint8_t drops;
    uint8_t highest_drop_height;
    uint8_t excitement;
    uint8_t intensity;
    uint8_t nausea;
    money16 upkeep_cost;
    uint8_t track_spine_colour[RCT12_NUM_COLOUR_SCHEMES];
    uint8_t track_rail_colour[RCT12_NUM_COLOUR_SCHEMES];
    uint8_t track_support_colour[RCT12_NUM_COLOUR_SCHEMES];
    uint32_t flags2;
    rct_object_entry vehicle_object;
    uint8_t space_required_x;
    uint8_t space_required_y;
    uint8_t vehicle_additional_colour[RCT2_MAX_CARS_PER_TRAIN];
    uint8_t lift_hill_speed;
    uint8_t num_circuits;

    std::vector<TrackDesignMazeElement> maze_elements;
    std::vector<TrackDesignTrackElement> track_elements;
    std::vector<TrackDesignEntranceElement> entrance_elements;
    std::vector<TrackDesignSceneryElement> scenery_elements;

    std::string name;

public:
    rct_string_id CreateTrackDesign(const Ride& ride);
    rct_string_id CreateTrackDesignScenery();
    void Serialise(DataSerialiser& stream);

private:
    uint8_t _saveDirection;
    rct_string_id CreateTrackDesignTrack(const Ride& ride);
    rct_string_id CreateTrackDesignMaze(const Ride& ride);
    CoordsXYE MazeGetFirstElement(const Ride& ride);
};

// Only written to in RCT2, not used in OpenRCT2. All of these are elements that had to be invented in RCT1.
enum : uint32_t
{
    TRACK_FLAGS_CONTAINS_VERTICAL_LOOP = (1 << 7),
    TRACK_FLAGS_CONTAINS_INLINE_TWIST = (1 << 17),
    TRACK_FLAGS_CONTAINS_HALF_LOOP = (1 << 18),
    TRACK_FLAGS_CONTAINS_CORKSCREW = (1 << 19),
    TRACK_FLAGS_CONTAINS_WATER_SPLASH = (1 << 27),
    TRACK_FLAGS_CONTAINS_BARREL_ROLL = (1 << 29),
    TRACK_FLAGS_CONTAINS_POWERED_LIFT = (1 << 30),
    TRACK_FLAGS_CONTAINS_LARGE_HALF_LOOP = (1u << 31),
};

enum : uint32_t
{
    TRACK_FLAGS2_CONTAINS_LOG_FLUME_REVERSER = (1 << 1),
    TRACK_FLAGS2_SIX_FLAGS_RIDE_DEPRECATED = (1u << 31) // Not used anymore.
};

enum
{
    TDPF_PLACE_SCENERY = 1 << 0,
};

enum
{
    TRACK_DESIGN_FLAG_SCENERY_UNAVAILABLE = (1 << 0),
    TRACK_DESIGN_FLAG_HAS_SCENERY = (1 << 1),
    TRACK_DESIGN_FLAG_VEHICLE_UNAVAILABLE = (1 << 2),
};

enum
{
    PTD_OPERATION_DRAW_OUTLINES,
    PTD_OPERATION_PLACE_QUERY,
    PTD_OPERATION_PLACE,
    PTD_OPERATION_GET_PLACE_Z,
    PTD_OPERATION_PLACE_GHOST,
    PTD_OPERATION_PLACE_TRACK_PREVIEW,
    PTD_OPERATION_REMOVE_GHOST,
};

static constexpr uint8_t PTD_OPERATION_FLAG_IS_REPLAY = (1 << 7);

enum
{
    MAZE_ELEMENT_TYPE_MAZE_TRACK = 0,
    MAZE_ELEMENT_TYPE_ENTRANCE = (1 << 3),
    MAZE_ELEMENT_TYPE_EXIT = (1 << 7)
};

extern TrackDesign* gActiveTrackDesign;
extern bool gTrackDesignSceneryToggle;

extern bool byte_9D8150;

extern bool _trackDesignPlaceStateSceneryUnavailable;
extern bool gTrackDesignSaveMode;
extern ride_id_t gTrackDesignSaveRideIndex;

std::unique_ptr<TrackDesign> track_design_open(const utf8* path);

void track_design_mirror(TrackDesign* td6);

int32_t place_virtual_track(
    TrackDesign* td6, uint8_t ptdOperation, bool placeScenery, Ride* ride, int16_t x, int16_t y, int16_t z);

///////////////////////////////////////////////////////////////////////////////
// Track design preview
///////////////////////////////////////////////////////////////////////////////
void track_design_draw_preview(TrackDesign* td6, uint8_t* pixels);

///////////////////////////////////////////////////////////////////////////////
// Track design saving
///////////////////////////////////////////////////////////////////////////////
void track_design_save_init();
void track_design_save_reset_scenery();
bool track_design_save_contains_tile_element(const TileElement* tileElement);
void track_design_save_select_nearby_scenery(ride_id_t rideIndex);
void track_design_save_select_tile_element(
    int32_t interactionType, const CoordsXY& loc, TileElement* tileElement, bool collect);

bool track_design_are_entrance_and_exit_placed();

extern std::vector<TrackDesignSceneryElement> _trackSavedTileElementsDesc;
extern std::vector<const TileElement*> _trackSavedTileElements;

#endif
