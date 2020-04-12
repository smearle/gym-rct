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
#include "../object/Object.h"
#include "../rct12/RCT12.h"
#include "../ride/RideRatings.h"
#include "../ride/Vehicle.h"
#include "../world/Location.hpp"

constexpr const uint8_t RCT2_MAX_STAFF = 200;
constexpr const uint8_t RCT2_MAX_BANNERS_IN_PARK = 250;
constexpr const uint8_t RCT2_MAX_VEHICLES_PER_RIDE = 31;
constexpr const uint8_t RCT2_MAX_CARS_PER_TRAIN = 32;
constexpr const uint8_t RCT2_MAX_CATEGORIES_PER_RIDE = 2;
constexpr const uint8_t RCT2_MAX_RIDE_TYPES_PER_RIDE_ENTRY = 3;
constexpr const uint8_t RCT2_MAX_VEHICLES_PER_RIDE_ENTRY = 4;
constexpr const uint8_t RCT2_DOWNTIME_HISTORY_SIZE = 8;
constexpr const uint8_t RCT2_CUSTOMER_HISTORY_SIZE = 10;
constexpr const uint16_t RCT2_MAX_SPRITES = 10000;
constexpr const uint32_t RCT2_MAX_TILE_ELEMENTS = 0x30000;
constexpr const uint16_t RCT2_MAX_ANIMATED_OBJECTS = 2000;
constexpr const uint8_t RCT2_MAX_RESEARCHED_RIDE_TYPE_QUADS = 8;  // With 32 bits per uint32_t, this means there is room for 256
                                                                  // types.
constexpr const uint8_t RCT2_MAX_RESEARCHED_RIDE_ENTRY_QUADS = 8; // With 32 bits per uint32_t, this means there is room for 256
                                                                  // entries.
constexpr const uint8_t RCT2_MAX_RESEARCHED_SCENERY_ITEM_QUADS = 56;
constexpr const uint16_t RCT2_MAX_RESEARCHED_SCENERY_ITEMS = (RCT2_MAX_RESEARCHED_SCENERY_ITEM_QUADS * 32); // There are 32 bits
                                                                                                            // per quad.
constexpr uint16_t TD6MaxTrackElements = 8192;

constexpr const uint8_t RCT2_MAX_RIDE_OBJECTS = 128;
constexpr const uint8_t RCT2_MAX_SMALL_SCENERY_OBJECTS = 252;
constexpr const uint8_t RCT2_MAX_LARGE_SCENERY_OBJECTS = 128;
constexpr const uint8_t RCT2_MAX_WALL_SCENERY_OBJECTS = 128;
constexpr const uint8_t RCT2_MAX_BANNER_OBJECTS = 32;
constexpr const uint8_t RCT2_MAX_PATH_OBJECTS = 16;
constexpr const uint8_t RCT2_MAX_PATH_ADDITION_OBJECTS = 15;
constexpr const uint8_t RCT2_MAX_SCENERY_GROUP_OBJECTS = 19;
constexpr const uint8_t RCT2_MAX_PARK_ENTRANCE_OBJECTS = 1;
constexpr const uint8_t RCT2_MAX_WATER_OBJECTS = 1;
constexpr const uint8_t RCT2_MAX_SCENARIO_TEXT_OBJECTS = 1;

// clang-format off
constexpr const uint16_t RCT2_OBJECT_ENTRY_COUNT =
    RCT2_MAX_RIDE_OBJECTS +
    RCT2_MAX_SMALL_SCENERY_OBJECTS +
    RCT2_MAX_LARGE_SCENERY_OBJECTS +
    RCT2_MAX_WALL_SCENERY_OBJECTS +
    RCT2_MAX_BANNER_OBJECTS +
    RCT2_MAX_PATH_OBJECTS +
    RCT2_MAX_PATH_ADDITION_OBJECTS +
    RCT2_MAX_SCENERY_GROUP_OBJECTS +
    RCT2_MAX_PARK_ENTRANCE_OBJECTS +
    RCT2_MAX_WATER_OBJECTS +
    RCT2_MAX_SCENARIO_TEXT_OBJECTS;
// clang-format on
static_assert(RCT2_OBJECT_ENTRY_COUNT == 721);

// clang-format off
constexpr const int32_t rct2_object_entry_group_counts[] = {
    RCT2_MAX_RIDE_OBJECTS,
    RCT2_MAX_SMALL_SCENERY_OBJECTS,
    RCT2_MAX_LARGE_SCENERY_OBJECTS,
    RCT2_MAX_WALL_SCENERY_OBJECTS,
    RCT2_MAX_BANNER_OBJECTS,
    RCT2_MAX_PATH_OBJECTS,
    RCT2_MAX_PATH_ADDITION_OBJECTS,
    RCT2_MAX_SCENERY_GROUP_OBJECTS,
    RCT2_MAX_PARK_ENTRANCE_OBJECTS,
    RCT2_MAX_WATER_OBJECTS,
    RCT2_MAX_SCENARIO_TEXT_OBJECTS,
};
// clang-format on

struct rct2_install_info
{
    uint32_t installLevel;
    char title[260];
    char path[260];
    uint32_t var_20C;
    uint8_t pad_210[256];
    char expansionPackNames[16][128];
    uint32_t activeExpansionPacks; // 0xB10
};

#pragma pack(push, 1)

/**
 * Ride structure.
 * size: 0x0260
 */
struct rct2_ride
{
    uint8_t type; // 0x000
    // pointer to static info. for example, wild mouse type is 0x36, subtype is
    // 0x4c.
    uint8_t subtype;                                             // 0x001
    uint16_t pad_002;                                            // 0x002
    uint8_t mode;                                                // 0x004
    uint8_t colour_scheme_type;                                  // 0x005
    rct_vehicle_colour vehicle_colours[RCT2_MAX_CARS_PER_TRAIN]; // 0x006
    uint8_t pad_046[0x03];                                       // 0x046, Used to be track colours in RCT1 without expansions
    // 0 = closed, 1 = open, 2 = test
    uint8_t status;     // 0x049
    rct_string_id name; // 0x04A
    union
    {
        uint32_t name_arguments; // 0x04C
        struct
        {
            rct_string_id name_arguments_type_name; // 0x04C
            uint16_t name_arguments_number;         // 0x04E
        };
    };
    RCT12xy8 overall_view;                                // 0x050
    RCT12xy8 station_starts[RCT12_MAX_STATIONS_PER_RIDE]; // 0x052
    uint8_t station_heights[RCT12_MAX_STATIONS_PER_RIDE]; // 0x05A
    uint8_t station_length[RCT12_MAX_STATIONS_PER_RIDE];  // 0x05E
    uint8_t station_depart[RCT12_MAX_STATIONS_PER_RIDE];  // 0x062
    // ride->vehicle index for current train waiting for passengers
    // at station
    uint8_t train_at_station[RCT12_MAX_STATIONS_PER_RIDE];    // 0x066
    RCT12xy8 entrances[RCT12_MAX_STATIONS_PER_RIDE];          // 0x06A
    RCT12xy8 exits[RCT12_MAX_STATIONS_PER_RIDE];              // 0x072
    uint16_t last_peep_in_queue[RCT12_MAX_STATIONS_PER_RIDE]; // 0x07A
    uint8_t pad_082[RCT12_MAX_STATIONS_PER_RIDE]; // 0x082, Used to be number of peeps in queue in RCT1, but this has moved.
    uint16_t vehicles[RCT2_MAX_VEHICLES_PER_RIDE + 1]; // 0x086, Points to the first car in the train
    uint8_t depart_flags;                              // 0x0C6

    // Not sure if these should be uint or sint.
    uint8_t num_stations;                // 0x0C7
    uint8_t num_vehicles;                // 0x0C8
    uint8_t num_cars_per_train;          // 0x0C9
    uint8_t proposed_num_vehicles;       // 0x0CA
    uint8_t proposed_num_cars_per_train; // 0x0CB
    uint8_t max_trains;                  // 0x0CC
    uint8_t min_max_cars_per_train;      // 0x0CD
    uint8_t min_waiting_time;            // 0x0CE
    uint8_t max_waiting_time;            // 0x0CF
    union
    {
        uint8_t operation_option; // 0x0D0
        uint8_t time_limit;       // 0x0D0
        uint8_t num_laps;         // 0x0D0
        uint8_t launch_speed;     // 0x0D0
        uint8_t speed;            // 0x0D0
        uint8_t rotations;        // 0x0D0
    };

    uint8_t boat_hire_return_direction; // 0x0D1
    RCT12xy8 boat_hire_return_position; // 0x0D2
    uint8_t measurement_index;          // 0x0D4
    // bits 0 through 4 are the number of helix sections
    // bit 5: spinning tunnel, water splash, or rapids
    // bit 6: log reverser, waterfall
    // bit 7: whirlpool
    uint8_t special_track_elements; // 0x0D5
    uint8_t pad_0D6[2];             // 0x0D6
    // Divide this value by 29127 to get the human-readable max speed
    // (in RCT2, display_speed = (max_speed * 9) >> 18)
    int32_t max_speed;                           // 0x0D8
    int32_t average_speed;                       // 0x0DC
    uint8_t current_test_segment;                // 0x0E0
    uint8_t average_speed_test_timeout;          // 0x0E1
    uint8_t pad_0E2[0x2];                        // 0x0E2
    int32_t length[RCT12_MAX_STATIONS_PER_RIDE]; // 0x0E4
    uint16_t time[RCT12_MAX_STATIONS_PER_RIDE];  // 0x0F4
    fixed16_2dp max_positive_vertical_g;         // 0x0FC
    fixed16_2dp max_negative_vertical_g;         // 0x0FE
    fixed16_2dp max_lateral_g;                   // 0x100
    fixed16_2dp previous_vertical_g;             // 0x102
    fixed16_2dp previous_lateral_g;              // 0x104
    uint8_t pad_106[0x2];                        // 0x106
    uint32_t testing_flags;                      // 0x108
    // x y map location of the current track piece during a test
    // this is to prevent counting special tracks multiple times
    RCT12xy8 cur_test_track_location; // 0x10C
    // Next 3 variables are related (XXXX XYYY ZZZa aaaa)
    uint16_t turn_count_default; // 0x10E X = current turn count
    uint16_t turn_count_banked;  // 0x110
    uint16_t turn_count_sloped;  // 0x112 X = number turns > 3 elements
    union
    {
        uint8_t inversions; // 0x114 (???X XXXX)
        uint8_t holes;      // 0x114 (???X XXXX)
        // This is a very rough approximation of how much of the ride is undercover.
        // It reaches the maximum value of 7 at about 50% undercover and doesn't increase beyond that.
        uint8_t sheltered_eighths; // 0x114 (XXX?-????)
    };
    // Y is number of powered lifts, X is drops
    uint8_t drops;               // 0x115 (YYXX XXXX)
    uint8_t start_drop_height;   // 0x116
    uint8_t highest_drop_height; // 0x117
    int32_t sheltered_length;    // 0x118
    // Unused always 0? Should affect nausea
    uint16_t var_11C;               // 0x11C
    uint8_t num_sheltered_sections; // 0x11E (?abY YYYY)
    // see cur_test_track_location
    uint8_t cur_test_track_z; // 0x11F
    // Customer counter in the current 960 game tick (about 30 seconds) interval
    uint16_t cur_num_customers; // 0x120
    // Counts ticks to update customer intervals, resets each 960 game ticks.
    uint16_t num_customers_timeout; // 0x122
    // Customer count in the last 10 * 960 game ticks (sliding window)
    uint16_t num_customers[RCT2_CUSTOMER_HISTORY_SIZE]; // 0x124
    money16 price;                                      // 0x138
    RCT12xy8 chairlift_bullwheel_location[2];           // 0x13A
    uint8_t chairlift_bullwheel_z[2];                   // 0x13E
    union
    {
        rating_tuple ratings; // 0x140
        struct
        {
            ride_rating excitement; // 0x140
            ride_rating intensity;  // 0x142
            ride_rating nausea;     // 0x144
        };
    };
    uint16_t value;                        // 0x146
    uint16_t chairlift_bullwheel_rotation; // 0x148
    uint8_t satisfaction;                  // 0x14A
    uint8_t satisfaction_time_out;         // 0x14B
    uint8_t satisfaction_next;             // 0x14C
    // Various flags stating whether a window needs to be refreshed
    uint8_t window_invalidate_flags; // 0x14D
    uint8_t pad_14E[0x02];           // 0x14E
    uint32_t total_customers;        // 0x150
    money32 total_profit;            // 0x154
    uint8_t popularity;              // 0x158
    uint8_t popularity_time_out;     // 0x159 Updated every purchase and ?possibly by time?
    uint8_t popularity_next;         // 0x15A When timeout reached this will be the next popularity
    uint8_t num_riders;              // 0x15B
    uint8_t music_tune_id;           // 0x15C
    uint8_t slide_in_use;            // 0x15D
    union
    {
        uint16_t slide_peep; // 0x15E
        uint16_t maze_tiles; // 0x15E
    };
    uint8_t pad_160[0xE];              // 0x160
    uint8_t slide_peep_t_shirt_colour; // 0x16E
    uint8_t pad_16F[0x7];              // 0x16F
    uint8_t spiral_slide_progress;     // 0x176
    uint8_t pad_177[0x9];              // 0x177
    int16_t build_date;                // 0x180
    money16 upkeep_cost;               // 0x182
    uint16_t race_winner;              // 0x184
    uint8_t pad_186[0x02];             // 0x186
    uint32_t music_position;           // 0x188
    uint8_t breakdown_reason_pending;  // 0x18C
    uint8_t mechanic_status;           // 0x18D
    uint16_t mechanic;                 // 0x18E
    uint8_t inspection_station;        // 0x190
    uint8_t broken_vehicle;            // 0x191
    uint8_t broken_car;                // 0x192
    uint8_t breakdown_reason;          // 0x193
    money16 price_secondary;           // 0x194
    union
    {
        struct
        {
            uint8_t reliability_subvalue;   // 0x196, 0 - 255, acts like the decimals for reliability_percentage
            uint8_t reliability_percentage; // 0x197, Starts at 100 and decreases from there.
        };
        uint16_t reliability; // 0x196
    };
    // Small constant used to increase the unreliability as the game continues,
    // making breakdowns more and more likely.
    uint8_t unreliability_factor; // 0x198
    // Range from [0, 100]
    uint8_t downtime;                                     // 0x199
    uint8_t inspection_interval;                          // 0x19A
    uint8_t last_inspection;                              // 0x19B
    uint8_t downtime_history[RCT2_DOWNTIME_HISTORY_SIZE]; // 0x19C
    uint32_t no_primary_items_sold;                       // 0x1A4
    uint32_t no_secondary_items_sold;                     // 0x1A8
    uint8_t breakdown_sound_modifier;                     // 0x1AC
    // Used to oscillate the sound when ride breaks down.
    // 0 = no change, 255 = max change
    uint8_t not_fixed_timeout;                                 // 0x1AD
    uint8_t last_crash_type;                                   // 0x1AE
    uint8_t connected_message_throttle;                        // 0x1AF
    money32 income_per_hour;                                   // 0x1B0
    money32 profit;                                            // 0x1B4
    uint8_t queue_time[RCT12_MAX_STATIONS_PER_RIDE];           // 0x1B8
    uint8_t track_colour_main[RCT12_NUM_COLOUR_SCHEMES];       // 0x1BC
    uint8_t track_colour_additional[RCT12_NUM_COLOUR_SCHEMES]; // 0x1C0
    uint8_t track_colour_supports[RCT12_NUM_COLOUR_SCHEMES];   // 0x1C4
    uint8_t music;                                             // 0x1C8
    uint8_t entrance_style;                                    // 0x1C9
    uint16_t vehicle_change_timeout;                           // 0x1CA
    uint8_t num_block_brakes;                                  // 0x1CC
    uint8_t lift_hill_speed;                                   // 0x1CD
    uint16_t guests_favourite;                                 // 0x1CE
    uint32_t lifecycle_flags;                                  // 0x1D0
    uint8_t vehicle_colours_extended[RCT2_MAX_CARS_PER_TRAIN]; // 0x1D4
    uint16_t total_air_time;                                   // 0x1F4
    uint8_t current_test_station;                              // 0x1F6
    uint8_t num_circuits;                                      // 0x1F7
    int16_t cable_lift_x;                                      // 0x1F8
    int16_t cable_lift_y;                                      // 0x1FA
    uint8_t cable_lift_z;                                      // 0x1FC
    uint8_t pad_1FD;                                           // 0x1FD
    uint16_t cable_lift;                                       // 0x1FE
    uint16_t queue_length[RCT12_MAX_STATIONS_PER_RIDE];        // 0x200
    uint8_t pad_208[0x58];                                     // 0x208
};
assert_struct_size(rct2_ride, 0x260);

/* Track Entrance entry size: 0x06 */
struct rct_td6_entrance_element
{
    int8_t z;          // 0x00
    uint8_t direction; // 0x01
    int16_t x;         // 0x02
    int16_t y;         // 0x04
};
assert_struct_size(rct_td6_entrance_element, 0x06);

/* Track Scenery entry  size: 0x16 */
struct rct_td6_scenery_element
{
    rct_object_entry scenery_object; // 0x00
    int8_t x;                        // 0x10
    int8_t y;                        // 0x11
    int8_t z;                        // 0x12
    uint8_t flags;                   // 0x13 direction quadrant tertiary colour
    uint8_t primary_colour;          // 0x14
    uint8_t secondary_colour;        // 0x15
};
assert_struct_size(rct_td6_scenery_element, 0x16);

/**
 * Track design structure.
 * size: 0xA3
 */
struct rct_track_td6
{
    uint8_t type; // 0x00
    uint8_t vehicle_type;
    union
    {
        // After loading the track this is converted to
        // a cost but before its a flags register
        money32 cost;   // 0x02
        uint32_t flags; // 0x02
    };
    union
    {
        // After loading the track this is converted to
        // a flags register
        uint8_t ride_mode;   // 0x06
        uint8_t track_flags; // 0x06
    };
    uint8_t version_and_colour_scheme;                           // 0x07 0b0000_VVCC
    rct_vehicle_colour vehicle_colours[RCT2_MAX_CARS_PER_TRAIN]; // 0x08
    union
    {
        uint8_t pad_48;
        uint8_t track_spine_colour_rct1; // 0x48
    };
    union
    {
        uint8_t entrance_style;         // 0x49
        uint8_t track_rail_colour_rct1; // 0x49
    };
    union
    {
        uint8_t total_air_time;            // 0x4A
        uint8_t track_support_colour_rct1; // 0x4A
    };
    uint8_t depart_flags;             // 0x4B
    uint8_t number_of_trains;         // 0x4C
    uint8_t number_of_cars_per_train; // 0x4D
    uint8_t min_waiting_time;         // 0x4E
    uint8_t max_waiting_time;         // 0x4F
    uint8_t operation_setting;
    int8_t max_speed;                // 0x51
    int8_t average_speed;            // 0x52
    uint16_t ride_length;            // 0x53
    uint8_t max_positive_vertical_g; // 0x55
    int8_t max_negative_vertical_g;  // 0x56
    uint8_t max_lateral_g;           // 0x57
    union
    {
        uint8_t inversions; // 0x58
        uint8_t holes;      // 0x58
    };
    uint8_t drops;                                              // 0x59
    uint8_t highest_drop_height;                                // 0x5A
    uint8_t excitement;                                         // 0x5B
    uint8_t intensity;                                          // 0x5C
    uint8_t nausea;                                             // 0x5D
    money16 upkeep_cost;                                        // 0x5E
    uint8_t track_spine_colour[RCT12_NUM_COLOUR_SCHEMES];       // 0x60
    uint8_t track_rail_colour[RCT12_NUM_COLOUR_SCHEMES];        // 0x64
    uint8_t track_support_colour[RCT12_NUM_COLOUR_SCHEMES];     // 0x68
    uint32_t flags2;                                            // 0x6C
    rct_object_entry vehicle_object;                            // 0x70
    uint8_t space_required_x;                                   // 0x80
    uint8_t space_required_y;                                   // 0x81
    uint8_t vehicle_additional_colour[RCT2_MAX_CARS_PER_TRAIN]; // 0x82
    uint8_t lift_hill_speed_num_circuits;                       // 0xA2 0bCCCL_LLLL
    // 0xA3 (data starts here in file)
};
assert_struct_size(rct_track_td6, 0xA3);

/**
 * scores.dat file header.
 * size: 0x10
 */
struct rct_scores_header
{
    uint32_t var_0;
    uint32_t var_4;
    uint32_t var_8;
    uint32_t ScenarioCount;
};
assert_struct_size(rct_scores_header, 0x10);

/**
 * An entry of scores.dat
 * size: 0x02B0
 */
struct rct_scores_entry
{
    char Path[256];
    uint8_t Category;
    uint8_t pad_0101[0x1F];
    int8_t ObjectiveType;
    int8_t ObjectiveArg1;
    int32_t objectiveArg2;
    int16_t objectiveArg3;
    char Name[64];
    char Details[256];
    int32_t Flags;
    money32 CompanyValue;
    char CompletedBy[64];
};
assert_struct_size(rct_scores_entry, 0x02B0);

struct RCT2SpriteVehicle : RCT12SpriteBase
{
    uint8_t vehicle_sprite_type; // 0x1F
    uint8_t bank_rotation;       // 0x20
    uint8_t pad_21[3];
    int32_t remaining_distance; // 0x24
    int32_t velocity;           // 0x28
    int32_t acceleration;       // 0x2C
    uint8_t ride;               // 0x30
    uint8_t vehicle_type;       // 0x31
    rct_vehicle_colour colours; // 0x32
    union
    {
        uint16_t track_progress; // 0x34
        struct
        {
            int8_t var_34;
            uint8_t var_35;
        };
    };
    union
    {
        int16_t track_direction; // 0x36
        int16_t track_type;      // 0x36
        RCT12xy8 boat_location;  // 0x36
    };
    uint16_t track_x;               // 0x38
    uint16_t track_y;               // 0x3A
    uint16_t track_z;               // 0x3C
    uint16_t next_vehicle_on_train; // 0x3E
    uint16_t prev_vehicle_on_ride;  // 0x40
    uint16_t next_vehicle_on_ride;  // 0x42
    uint16_t var_44;
    uint16_t mass;         // 0x46
    uint16_t update_flags; // 0x48
    uint8_t swing_sprite;
    uint8_t current_station; // 0x4B
    union
    {
        int16_t swinging_car_var_0; // 0x4C
        int16_t current_time;       // 0x4C
        struct
        {
            int8_t ferris_wheel_var_0; // 0x4C
            int8_t ferris_wheel_var_1; // 0x4D
        };
    };
    union
    {
        int16_t var_4E;
        int16_t crash_z; // 0x4E
    };
    uint8_t status;                  // 0x50
    uint8_t sub_state;               // 0x51
    uint16_t peep[32];               // 0x52
    uint8_t peep_tshirt_colours[32]; // 0x92
    uint8_t num_seats;               // 0xB2
    uint8_t num_peeps;               // 0xB3
    uint8_t next_free_seat;          // 0xB4
    uint8_t restraints_position;     // 0xB5
    union
    {
        int16_t spin_speed; // 0xB6
        int16_t crash_x;    // 0xB6
    };
    uint16_t sound2_flags; // 0xB8
    uint8_t spin_sprite;   // 0xBA
    uint8_t sound1_id;     // 0xBB
    uint8_t sound1_volume; // 0xBC
    uint8_t sound2_id;     // 0xBD
    uint8_t sound2_volume; // 0xBE
    int8_t sound_vector_factor;
    union
    {
        uint16_t var_C0;
        int16_t crash_y;            // 0xC0
        uint16_t time_waiting;      // 0xC0
        uint16_t cable_lift_target; // 0xC0
    };
    uint8_t speed;                // 0xC2
    uint8_t powered_acceleration; // 0xC3
    union
    {
        uint8_t dodgems_collision_direction; // 0xC4
        uint8_t var_C4;
    };
    uint8_t animation_frame; // 0xC5
    uint8_t pad_C6[0x2];
    uint16_t var_C8;
    uint16_t var_CA;
    uint8_t scream_sound_id; // 0xCC
    uint8_t TrackSubposition;
    union
    {
        uint8_t var_CE;
        uint8_t num_laps; // 0xCE
    };
    union
    {
        uint8_t var_CF;
        uint8_t brake_speed; // 0xCF
    };
    uint16_t lost_time_out;         // 0xD0
    int8_t vertical_drop_countdown; // 0xD1
    uint8_t var_D3;
    uint8_t mini_golf_current_animation;
    uint8_t mini_golf_flags;      // 0xD5
    uint8_t ride_subtype;         // 0xD6
    uint8_t colours_extended;     // 0xD7
    uint8_t seat_rotation;        // 0xD8
    uint8_t target_seat_rotation; // 0xD9
};
assert_struct_size(RCT2SpriteVehicle, 0xDA);

struct RCT2SpritePeep : RCT12SpriteBase
{
    uint8_t pad_1F[0x22 - 0x1F];
    rct_string_id name_string_idx; // 0x22
    uint16_t next_x;               // 0x24
    uint16_t next_y;               // 0x26
    uint8_t next_z;                // 0x28
    uint8_t next_flags;            // 0x29
    uint8_t outside_of_park;       // 0x2A
    uint8_t state;                 // 0x2B
    uint8_t sub_state;             // 0x2C
    uint8_t sprite_type;           // 0x2D
    uint8_t peep_type;             // 0x2E
    union
    {
        uint8_t staff_type;  // 0x2F
        uint8_t no_of_rides; // 0x2F
    };
    uint8_t tshirt_colour;         // 0x30
    uint8_t trousers_colour;       // 0x31
    uint16_t destination_x;        // 0x32
    uint16_t destination_y;        // 0x34
    uint8_t destination_tolerance; // 0x36
    uint8_t var_37;
    uint8_t energy;                  // 0x38
    uint8_t energy_target;           // 0x39
    uint8_t happiness;               // 0x3A
    uint8_t happiness_target;        // 0x3B
    uint8_t nausea;                  // 0x3C
    uint8_t nausea_target;           // 0x3D
    uint8_t hunger;                  // 0x3E
    uint8_t thirst;                  // 0x3F
    uint8_t toilet;                  // 0x40
    uint8_t mass;                    // 0x41
    uint8_t time_to_consume;         // 0x42
    uint8_t intensity;               // 0x43
    uint8_t nausea_tolerance;        // 0x44
    uint8_t window_invalidate_flags; // 0x45
    money16 paid_on_drink;           // 0x46
    uint8_t ride_types_been_on[16];  // 0x48
    uint32_t item_extra_flags;       // 0x58
    uint8_t photo2_ride_ref;         // 0x5C
    uint8_t photo3_ride_ref;         // 0x5D
    uint8_t photo4_ride_ref;         // 0x5E
    uint8_t pad_5F[0x09];            // 0x5F
    uint8_t current_ride;            // 0x68
    uint8_t current_ride_station;    // 0x69
    uint8_t current_train;           // 0x6A
    union
    {
        struct
        {
            uint8_t current_car;  // 0x6B
            uint8_t current_seat; // 0x6C
        };
        uint16_t time_to_sitdown; // 0x6B
        struct
        {
            uint8_t time_to_stand;  // 0x6B
            uint8_t standing_flags; // 0x6C
        };
    };
    uint8_t special_sprite;             // 0x6D
    uint8_t action_sprite_type;         // 0x6E
    uint8_t next_action_sprite_type;    // 0x6F
    uint8_t action_sprite_image_offset; // 0x70
    uint8_t action;                     // 0x71
    uint8_t action_frame;               // 0x72
    uint8_t step_progress;              // 0x73
    union
    {
        uint16_t mechanic_time_since_call;
        uint16_t next_in_queue; // 0x74
    };
    uint8_t pad_76;
    uint8_t pad_77;
    union
    {
        uint8_t maze_last_edge; // 0x78
        uint8_t direction;
    };
    uint8_t interaction_ride_index;
    uint16_t time_in_queue;                             // 0x7A
    uint8_t rides_been_on[32];                          // 0x7C
    uint32_t id;                                        // 0x9C
    money32 cash_in_pocket;                             // 0xA0
    money32 cash_spent;                                 // 0xA4
    int32_t time_in_park;                               // 0xA8
    int8_t rejoin_queue_timeout;                        // 0xAC
    uint8_t previous_ride;                              // 0xAD
    uint16_t previous_ride_time_out;                    // 0xAE
    RCT12PeepThought thoughts[RCT12_PEEP_MAX_THOUGHTS]; // 0xB0
    uint8_t path_check_optimisation;                    // 0xC4
    union
    {
        uint8_t staff_id;                 // 0xC5
        uint8_t guest_heading_to_ride_id; // 0xC5
    };
    union
    {
        uint8_t staff_orders;           // 0xC6
        uint8_t peep_is_lost_countdown; // 0xC6
    };
    uint8_t photo1_ride_ref;         // 0xC7
    uint32_t peep_flags;             // 0xC8
    rct12_xyzd8 pathfind_goal;       // 0xCC
    rct12_xyzd8 pathfind_history[4]; // 0xD0
    uint8_t no_action_frame_num;     // 0xE0
    uint8_t litter_count;            // 0xE1
    union
    {
        uint8_t time_on_ride;         // 0xE2
        uint8_t staff_mowing_timeout; // 0xE2
    };
    uint8_t disgusting_count; // 0xE3
    union
    {
        money16 paid_to_enter;      // 0xE4
        uint16_t staff_lawns_mown;  // 0xE4
        uint16_t staff_rides_fixed; // 0xE4
    };
    union
    {
        money16 paid_on_rides;          // 0xE6
        uint16_t staff_gardens_watered; // 0xE6
        uint16_t staff_rides_inspected; // 0xE6
    };
    union
    {
        money16 paid_on_food;        // 0xE8
        uint16_t staff_litter_swept; // 0xE8
    };
    union
    {
        money16 paid_on_souvenirs;   // 0xEA
        uint16_t staff_bins_emptied; // 0xEA
    };
    uint8_t no_of_food;                   // 0xEC
    uint8_t no_of_drinks;                 // 0xED
    uint8_t no_of_souvenirs;              // 0xEE
    uint8_t vandalism_seen;               // 0xEF 0xC0 vandalism thought timeout, 0x3F vandalism tiles seen
    uint8_t voucher_type;                 // 0xF0
    uint8_t voucher_arguments;            // 0xF1 ride_id or string_offset_id
    uint8_t surroundings_thought_timeout; // 0xF2
    uint8_t angriness;                    // 0xF3
    uint8_t time_lost;                    // 0xF4 the time the peep has been lost when it reaches 254 generates the lost thought
    uint8_t days_in_queue;                // 0xF5
    uint8_t balloon_colour;               // 0xF6
    uint8_t umbrella_colour;              // 0xF7
    uint8_t hat_colour;                   // 0xF8
    uint8_t favourite_ride;               // 0xF9
    uint8_t favourite_ride_rating;        // 0xFA
    uint8_t pad_FB;
    uint32_t item_standard_flags; // 0xFC
};
assert_struct_size(RCT2SpritePeep, 0x100);

union RCT2Sprite
{
private:
    uint8_t pad_00[0x100];

public:
    RCT12SpriteBase unknown;
    RCT2SpriteVehicle vehicle;
    RCT2SpritePeep peep;
    RCT12SpriteLitter litter;
    RCT12SpriteBalloon balloon;
    RCT12SpriteDuck duck;
    RCT12SpriteJumpingFountain jumping_fountain;
    RCT12SpriteMoneyEffect money_effect;
    RCT12SpriteCrashedVehicleParticle crashed_vehicle_particle;
    RCT12SpriteCrashSplash crash_splash;
    RCT12SpriteSteamParticle steam_particle;
};
assert_struct_size(RCT2Sprite, 0x100);

struct RCT2RideRatingCalculationData
{
    uint16_t proximity_x;
    uint16_t proximity_y;
    uint16_t proximity_z;
    uint16_t proximity_start_x;
    uint16_t proximity_start_y;
    uint16_t proximity_start_z;
    uint8_t current_ride;
    uint8_t state;
    uint8_t proximity_track_type;
    uint8_t proximity_base_height;
    uint16_t proximity_total;
    uint16_t proximity_scores[26];
    uint16_t num_brakes;
    uint16_t num_reversers;
    uint16_t station_flags;
};
assert_struct_size(RCT2RideRatingCalculationData, 76);

#pragma pack(pop)
