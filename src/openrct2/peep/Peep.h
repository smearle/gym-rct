/*****************************************************************************
 * Copyright (c) 2014-2019 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#ifndef _PEEP_H_
#define _PEEP_H_

#include "../common.h"
#include "../management/Finance.h"
#include "../rct12/RCT12.h"
#include "../ride/Ride.h"
#include "../ride/RideTypes.h"
#include "../world/Location.hpp"
#include "../world/SpriteBase.h"

#include <bitset>
#include <optional>

#define PEEP_MAX_THOUGHTS 5
#define PEEP_THOUGHT_ITEM_NONE 255

#define PEEP_HUNGER_WARNING_THRESHOLD 25
#define PEEP_THIRST_WARNING_THRESHOLD 25
#define PEEP_BATHROOM_WARNING_THRESHOLD 28
#define PEEP_LITTER_WARNING_THRESHOLD 23
#define PEEP_DISGUST_WARNING_THRESHOLD 22
#define PEEP_VANDALISM_WARNING_THRESHOLD 15
#define PEEP_NOEXIT_WARNING_THRESHOLD 8
#define PEEP_LOST_WARNING_THRESHOLD 8

#define PEEP_MAX_HAPPINESS 255
#define PEEP_MIN_ENERGY 32
#define PEEP_MAX_ENERGY 128
#define PEEP_MAX_ENERGY_TARGET 255 // Oddly, this differs from max energy!
#define PEEP_MAX_HUNGER 255
#define PEEP_MAX_BATHROOM 255
#define PEEP_MAX_NAUSEA 255
#define PEEP_MAX_THIRST 255

constexpr auto PEEP_CLEARANCE_HEIGHT = 4 * COORDS_Z_STEP;

struct TileElement;
struct Ride;

enum PeepType : uint8_t
{
    PEEP_TYPE_GUEST,
    PEEP_TYPE_STAFF,

    PEEP_TYPE_INVALID = 0xFF
};

enum PeepThoughtType : uint8_t
{
    PEEP_THOUGHT_TYPE_CANT_AFFORD_0 = 0,      // "I can't afford"
    PEEP_THOUGHT_TYPE_SPENT_MONEY = 1,        // "I've spent all my money"
    PEEP_THOUGHT_TYPE_SICK = 2,               // "I feel sick"
    PEEP_THOUGHT_TYPE_VERY_SICK = 3,          // "I feel very sick"
    PEEP_THOUGHT_TYPE_MORE_THRILLING = 4,     // "I want to go on something more thrilling than X"
    PEEP_THOUGHT_TYPE_INTENSE = 5,            // "X looks too intense for me"
    PEEP_THOUGHT_TYPE_HAVENT_FINISHED = 6,    // "I haven't finished my X yet"
    PEEP_THOUGHT_TYPE_SICKENING = 7,          // "Just looking at X makes me feel sick"
    PEEP_THOUGHT_TYPE_BAD_VALUE = 8,          // "I'm not paying that much to go on X"
    PEEP_THOUGHT_TYPE_GO_HOME = 9,            // "I want to go home"
    PEEP_THOUGHT_TYPE_GOOD_VALUE = 10,        // "X is really good value"
    PEEP_THOUGHT_TYPE_ALREADY_GOT = 11,       // "I've already got"
    PEEP_THOUGHT_TYPE_CANT_AFFORD = 12,       // "I can't afford"
    PEEP_THOUGHT_TYPE_NOT_HUNGRY = 13,        // "I'm not hungry"
    PEEP_THOUGHT_TYPE_NOT_THIRSTY = 14,       // "I'm not thirsty"
    PEEP_THOUGHT_TYPE_DROWNING = 15,          // "Help! I'm drowning!"
    PEEP_THOUGHT_TYPE_LOST = 16,              // "I'm lost!"
    PEEP_THOUGHT_TYPE_WAS_GREAT = 17,         // "X was great"
    PEEP_THOUGHT_TYPE_QUEUING_AGES = 18,      // "I've been queuing for X for ages"
    PEEP_THOUGHT_TYPE_TIRED = 19,             // "I'm tired"
    PEEP_THOUGHT_TYPE_HUNGRY = 20,            // "I'm hungry"
    PEEP_THOUGHT_TYPE_THIRSTY = 21,           // "I'm thirsty"
    PEEP_THOUGHT_TYPE_BATHROOM = 22,          // "I need to go to the bathroom"
    PEEP_THOUGHT_TYPE_CANT_FIND = 23,         // "I can't find X"
    PEEP_THOUGHT_TYPE_NOT_PAYING = 24,        // "I'm not paying that much to use X"
    PEEP_THOUGHT_TYPE_NOT_WHILE_RAINING = 25, // "I'm not going on X while it's raining"
    PEEP_THOUGHT_TYPE_BAD_LITTER = 26,        // "The litter here is really bad"
    PEEP_THOUGHT_TYPE_CANT_FIND_EXIT = 27,    // "I can't find the exit"
    PEEP_THOUGHT_TYPE_GET_OFF = 28,           // "I want to get off X"
    PEEP_THOUGHT_TYPE_GET_OUT = 29,           // "I want to get out of X"
    PEEP_THOUGHT_TYPE_NOT_SAFE = 30,          // "I'm not going on X - it isn't safe"
    PEEP_THOUGHT_TYPE_PATH_DISGUSTING = 31,   // "This path is disgusting"
    PEEP_THOUGHT_TYPE_CROWDED = 32,           // "It's too crowded here"
    PEEP_THOUGHT_TYPE_VANDALISM = 33,         // "The vandalism here is really bad"
    PEEP_THOUGHT_TYPE_SCENERY = 34,           // "Great scenery!"
    PEEP_THOUGHT_TYPE_VERY_CLEAN = 35,        // "This park is very clean and tidy"
    PEEP_THOUGHT_TYPE_FOUNTAINS = 36,         // "The jumping fountains are great"
    PEEP_THOUGHT_TYPE_MUSIC = 37,             // "The music is nice here"
    PEEP_THOUGHT_TYPE_BALLOON = 38,           // "This balloon from X is really good value"
    PEEP_THOUGHT_TYPE_TOY = 39,               // "This cuddly toy from X is really good value"
    PEEP_THOUGHT_TYPE_MAP = 40,
    PEEP_THOUGHT_TYPE_PHOTO = 41, // "This on ride photo from X is really good value"
    PEEP_THOUGHT_TYPE_UMBRELLA = 42,
    PEEP_THOUGHT_TYPE_DRINK = 43,
    PEEP_THOUGHT_TYPE_BURGER = 44,
    PEEP_THOUGHT_TYPE_CHIPS = 45,
    PEEP_THOUGHT_TYPE_ICE_CREAM = 46,
    PEEP_THOUGHT_TYPE_CANDYFLOSS = 47,

    PEEP_THOUGHT_TYPE_PIZZA = 51,

    PEEP_THOUGHT_TYPE_POPCORN = 53,
    PEEP_THOUGHT_TYPE_HOT_DOG = 54,
    PEEP_THOUGHT_TYPE_TENTACLE = 55,
    PEEP_THOUGHT_TYPE_HAT = 56,
    PEEP_THOUGHT_TYPE_TOFFEE_APPLE = 57,
    PEEP_THOUGHT_TYPE_TSHIRT = 58,
    PEEP_THOUGHT_TYPE_DOUGHNUT = 59,
    PEEP_THOUGHT_TYPE_COFFEE = 60,

    PEEP_THOUGHT_TYPE_CHICKEN = 62,
    PEEP_THOUGHT_TYPE_LEMONADE = 63,

    PEEP_THOUGHT_TYPE_WOW = 67, // "Wow!"

    PEEP_THOUGHT_TYPE_WOW2 = 70,         // "Wow!"
    PEEP_THOUGHT_TYPE_WATCHED = 71,      // "I have the strangest feeling someone is watching me"
    PEEP_THOUGHT_TYPE_BALLOON_MUCH = 72, // "I'm not paying that much to get a balloon from X"
    PEEP_THOUGHT_TYPE_TOY_MUCH = 73,
    PEEP_THOUGHT_TYPE_MAP_MUCH = 74,
    PEEP_THOUGHT_TYPE_PHOTO_MUCH = 75,
    PEEP_THOUGHT_TYPE_UMBRELLA_MUCH = 76,
    PEEP_THOUGHT_TYPE_DRINK_MUCH = 77,
    PEEP_THOUGHT_TYPE_BURGER_MUCH = 78,
    PEEP_THOUGHT_TYPE_CHIPS_MUCH = 79,
    PEEP_THOUGHT_TYPE_ICE_CREAM_MUCH = 80,
    PEEP_THOUGHT_TYPE_CANDYFLOSS_MUCH = 81,

    PEEP_THOUGHT_TYPE_PIZZA_MUCH = 85,

    PEEP_THOUGHT_TYPE_POPCORN_MUCH = 87,
    PEEP_THOUGHT_TYPE_HOT_DOG_MUCH = 88,
    PEEP_THOUGHT_TYPE_TENTACLE_MUCH = 89,
    PEEP_THOUGHT_TYPE_HAT_MUCH = 90,
    PEEP_THOUGHT_TYPE_TOFFEE_APPLE_MUCH = 91,
    PEEP_THOUGHT_TYPE_TSHIRT_MUCH = 92,
    PEEP_THOUGHT_TYPE_DOUGHNUT_MUCH = 93,
    PEEP_THOUGHT_TYPE_COFFEE_MUCH = 94,

    PEEP_THOUGHT_TYPE_CHICKEN_MUCH = 96,
    PEEP_THOUGHT_TYPE_LEMONADE_MUCH = 97,

    PEEP_THOUGHT_TYPE_PHOTO2 = 104, // "This on-ride photo from X is really good value"
    PEEP_THOUGHT_TYPE_PHOTO3 = 105,
    PEEP_THOUGHT_TYPE_PHOTO4 = 106,
    PEEP_THOUGHT_TYPE_PRETZEL = 107,
    PEEP_THOUGHT_TYPE_HOT_CHOCOLATE = 108,
    PEEP_THOUGHT_TYPE_ICED_TEA = 109,
    PEEP_THOUGHT_TYPE_FUNNEL_CAKE = 110,
    PEEP_THOUGHT_TYPE_SUNGLASSES = 111,
    PEEP_THOUGHT_TYPE_BEEF_NOODLES = 112,
    PEEP_THOUGHT_TYPE_FRIED_RICE_NOODLES = 113,
    PEEP_THOUGHT_TYPE_WONTON_SOUP = 114,
    PEEP_THOUGHT_TYPE_MEATBALL_SOUP = 115,
    PEEP_THOUGHT_TYPE_FRUIT_JUICE = 116,
    PEEP_THOUGHT_TYPE_SOYBEAN_MILK = 117,
    PEEP_THOUGHT_TYPE_SU_JONGKWA = 118,
    PEEP_THOUGHT_TYPE_SUB_SANDWICH = 119,
    PEEP_THOUGHT_TYPE_COOKIE = 120,

    PEEP_THOUGHT_TYPE_ROAST_SAUSAGE = 124,

    PEEP_THOUGHT_TYPE_PHOTO2_MUCH = 136,
    PEEP_THOUGHT_TYPE_PHOTO3_MUCH = 137,
    PEEP_THOUGHT_TYPE_PHOTO4_MUCH = 138,
    PEEP_THOUGHT_TYPE_PRETZEL_MUCH = 139,
    PEEP_THOUGHT_TYPE_HOT_CHOCOLATE_MUCH = 140,
    PEEP_THOUGHT_TYPE_ICED_TEA_MUCH = 141,
    PEEP_THOUGHT_TYPE_FUNNEL_CAKE_MUCH = 142,
    PEEP_THOUGHT_TYPE_SUNGLASSES_MUCH = 143,
    PEEP_THOUGHT_TYPE_BEEF_NOODLES_MUCH = 144,
    PEEP_THOUGHT_TYPE_FRIED_RICE_NOODLES_MUCH = 145,
    PEEP_THOUGHT_TYPE_WONTON_SOUP_MUCH = 146,
    PEEP_THOUGHT_TYPE_MEATBALL_SOUP_MUCH = 147,
    PEEP_THOUGHT_TYPE_FRUIT_JUICE_MUCH = 148,
    PEEP_THOUGHT_TYPE_SOYBEAN_MILK_MUCH = 149,
    PEEP_THOUGHT_TYPE_SU_JONGKWA_MUCH = 150,
    PEEP_THOUGHT_TYPE_SUB_SANDWICH_MUCH = 151,
    PEEP_THOUGHT_TYPE_COOKIE_MUCH = 152,

    PEEP_THOUGHT_TYPE_ROAST_SAUSAGE_MUCH = 156,

    PEEP_THOUGHT_TYPE_HELP = 168,                 // "Help! Put me down!"
    PEEP_THOUGHT_TYPE_RUNNING_OUT = 169,          // "I'm running out of cash!"
    PEEP_THOUGHT_TYPE_NEW_RIDE = 170,             // "Wow! A new ride being built!"
    PEEP_THOUGHT_TYPE_NICE_RIDE_DEPRECATED = 171, // "Nice ride! But not as good as the Phoenix..."
    PEEP_THOUGHT_TYPE_EXCITED_DEPRECATED = 172,   // "I'm so excited - It's an Intamin ride!"
    PEEP_THOUGHT_TYPE_HERE_WE_ARE = 173,          // "...and here we are on X!"

    PEEP_THOUGHT_TYPE_NONE = 255
};

enum PeepState : uint8_t
{
    PEEP_STATE_FALLING = 0, // Drowning is part of falling
    PEEP_STATE_1 = 1,
    PEEP_STATE_QUEUING_FRONT = 2,
    PEEP_STATE_ON_RIDE = 3,
    PEEP_STATE_LEAVING_RIDE = 4,
    PEEP_STATE_WALKING = 5,
    PEEP_STATE_QUEUING = 6,
    PEEP_STATE_ENTERING_RIDE = 7,
    PEEP_STATE_SITTING = 8,
    PEEP_STATE_PICKED = 9,
    PEEP_STATE_PATROLLING = 10, // Not sure
    PEEP_STATE_MOWING = 11,
    PEEP_STATE_SWEEPING = 12,
    PEEP_STATE_ENTERING_PARK = 13,
    PEEP_STATE_LEAVING_PARK = 14,
    PEEP_STATE_ANSWERING = 15,
    PEEP_STATE_FIXING = 16,
    PEEP_STATE_BUYING = 17,
    PEEP_STATE_WATCHING = 18,
    PEEP_STATE_EMPTYING_BIN = 19,
    PEEP_STATE_USING_BIN = 20,
    PEEP_STATE_WATERING = 21,
    PEEP_STATE_HEADING_TO_INSPECTION = 22,
    PEEP_STATE_INSPECTING = 23
};

enum PeepSittingSubState
{
    PEEP_SITTING_TRYING_TO_SIT = 0,
    PEEP_SITTING_SAT_DOWN
};

enum PeepRideSubState
{
    PEEP_RIDE_AT_ENTRANCE = 0,
    PEEP_RIDE_IN_ENTRANCE = 1,
    PEEP_RIDE_FREE_VEHICLE_CHECK = 2, // Spend money on ride
    PEEP_RIDE_LEAVE_ENTRANCE = 3,     // Calculate what direction and where to go after commiting to entering vehicle
    PEEP_RIDE_APPROACH_VEHICLE = 4,
    PEEP_RIDE_ENTER_VEHICLE = 5,
    PEEP_RIDE_ON_RIDE = 6,
    PEEP_RIDE_LEAVE_VEHICLE = 7,
    PEEP_RIDE_APPROACH_EXIT = 8,
    PEEP_RIDE_IN_EXIT = 9,
    // 10, 11 not used
    PEEP_RIDE_APPROACH_VEHICLE_WAYPOINTS = 12,
    PEEP_RIDE_APPROACH_EXIT_WAYPOINTS = 13,
    PEEP_RIDE_APPROACH_SPIRAL_SLIDE = 14,
    PEEP_RIDE_ON_SPIRAL_SLIDE = 15,
    PEEP_RIDE_LEAVE_SPIRAL_SLIDE = 16,
    PEEP_RIDE_MAZE_PATHFINDING = 17,
    PEEP_RIDE_LEAVE_EXIT = 18,
    PEEP_SHOP_APPROACH = 19,
    PEEP_SHOP_INTERACT = 20,
    PEEP_SHOP_LEAVE = 21
};

enum PeepUsingBinSubState
{
    PEEP_USING_BIN_WALKING_TO_BIN = 0,
    PEEP_USING_BIN_GOING_BACK,
};

enum PeepActionType : uint8_t
{
    PEEP_ACTION_CHECK_TIME = 0,
    // If no food then check watch
    PEEP_ACTION_EAT_FOOD = 1,
    PEEP_ACTION_SHAKE_HEAD = 2,
    PEEP_ACTION_EMPTY_POCKETS = 3,
    PEEP_ACTION_SITTING_EAT_FOOD = 4,
    PEEP_ACTION_SITTING_CHECK_WATCH = 4,
    PEEP_ACTION_SITTING_LOOK_AROUND_LEFT = 5,
    PEEP_ACTION_SITTING_LOOK_AROUND_RIGHT = 6,
    PEEP_ACTION_WOW = 7,
    PEEP_ACTION_THROW_UP = 8,
    PEEP_ACTION_JUMP = 9,
    PEEP_ACTION_STAFF_SWEEP = 10,
    PEEP_ACTION_DROWNING = 11,
    PEEP_ACTION_STAFF_ANSWER_CALL = 12,
    PEEP_ACTION_STAFF_ANSWER_CALL_2 = 13,
    PEEP_ACTION_STAFF_CHECKBOARD = 14,
    PEEP_ACTION_STAFF_FIX = 15,
    PEEP_ACTION_STAFF_FIX_2 = 16,
    PEEP_ACTION_STAFF_FIX_GROUND = 17,
    PEEP_ACTION_STAFF_FIX_3 = 18,
    PEEP_ACTION_STAFF_WATERING = 19,
    PEEP_ACTION_JOY = 20,
    PEEP_ACTION_READ_MAP = 21,
    PEEP_ACTION_WAVE = 22,
    PEEP_ACTION_STAFF_EMPTY_BIN = 23,
    PEEP_ACTION_WAVE_2 = 24,
    PEEP_ACTION_TAKE_PHOTO = 25,
    PEEP_ACTION_CLAP = 26,
    PEEP_ACTION_DISGUST = 27,
    PEEP_ACTION_DRAW_PICTURE = 28,
    PEEP_ACTION_BEING_WATCHED = 29,
    PEEP_ACTION_WITHDRAW_MONEY = 30,

    PEEP_ACTION_NONE_1 = 254,
    PEEP_ACTION_NONE_2 = 255,
};

enum PeepActionSpriteType : uint8_t
{
    PEEP_ACTION_SPRITE_TYPE_NONE = 0,
    PEEP_ACTION_SPRITE_TYPE_CHECK_TIME = 1,
    PEEP_ACTION_SPRITE_TYPE_WATCH_RIDE = 2,
    PEEP_ACTION_SPRITE_TYPE_EAT_FOOD = 3,
    PEEP_ACTION_SPRITE_TYPE_SHAKE_HEAD = 4,
    PEEP_ACTION_SPRITE_TYPE_EMPTY_POCKETS = 5,
    PEEP_ACTION_SPRITE_TYPE_HOLD_MAT = 6,
    PEEP_ACTION_SPRITE_TYPE_SITTING_IDLE = 7,
    PEEP_ACTION_SPRITE_TYPE_SITTING_EAT_FOOD = 8,
    PEEP_ACTION_SPRITE_TYPE_SITTING_LOOK_AROUND_LEFT = 9,
    PEEP_ACTION_SPRITE_TYPE_SITTING_LOOK_AROUND_RIGHT = 10,
    PEEP_ACTION_SPRITE_TYPE_UI = 11,
    PEEP_ACTION_SPRITE_TYPE_STAFF_MOWER = 12,
    PEEP_ACTION_SPRITE_TYPE_WOW = 13,
    PEEP_ACTION_SPRITE_TYPE_THROW_UP = 14,
    PEEP_ACTION_SPRITE_TYPE_JUMP = 15,
    PEEP_ACTION_SPRITE_TYPE_STAFF_SWEEP = 16,
    PEEP_ACTION_SPRITE_TYPE_DROWNING = 17,
    PEEP_ACTION_SPRITE_TYPE_STAFF_ANSWER_CALL = 18,
    PEEP_ACTION_SPRITE_TYPE_STAFF_ANSWER_CALL_2 = 19,
    PEEP_ACTION_SPRITE_TYPE_STAFF_CHECKBOARD = 20,
    PEEP_ACTION_SPRITE_TYPE_STAFF_FIX = 21,
    PEEP_ACTION_SPRITE_TYPE_STAFF_FIX_2 = 22,
    PEEP_ACTION_SPRITE_TYPE_STAFF_FIX_GROUND = 23,
    PEEP_ACTION_SPRITE_TYPE_STAFF_FIX_3 = 24,
    PEEP_ACTION_SPRITE_TYPE_STAFF_WATERING = 25,
    PEEP_ACTION_SPRITE_TYPE_JOY = 26,
    PEEP_ACTION_SPRITE_TYPE_READ_MAP = 27,
    PEEP_ACTION_SPRITE_TYPE_WAVE = 28,
    PEEP_ACTION_SPRITE_TYPE_STAFF_EMPTY_BIN = 29,
    PEEP_ACTION_SPRITE_TYPE_WAVE_2 = 30,
    PEEP_ACTION_SPRITE_TYPE_TAKE_PHOTO = 31,
    PEEP_ACTION_SPRITE_TYPE_CLAP = 32,
    PEEP_ACTION_SPRITE_TYPE_DISGUST = 33,
    PEEP_ACTION_SPRITE_TYPE_DRAW_PICTURE = 34,
    PEEP_ACTION_SPRITE_TYPE_BEING_WATCHED = 35,
    PEEP_ACTION_SPRITE_TYPE_WITHDRAW_MONEY = 36,

    PEEP_ACTION_SPRITE_TYPE_INVALID = 255
};

enum PeepFlags : uint32_t
{
    PEEP_FLAGS_LEAVING_PARK = (1 << 0),
    PEEP_FLAGS_SLOW_WALK = (1 << 1),
    PEEP_FLAGS_2 = (1 << 2),
    PEEP_FLAGS_TRACKING = (1 << 3),
    PEEP_FLAGS_WAVING = (1 << 4),                  // Makes the peep wave
    PEEP_FLAGS_HAS_PAID_FOR_PARK_ENTRY = (1 << 5), // Set on paying to enter park
    PEEP_FLAGS_PHOTO = (1 << 6),                   // Makes the peep take a picture
    PEEP_FLAGS_PAINTING = (1 << 7),
    PEEP_FLAGS_WOW = (1 << 8),        // Makes a peep WOW2
    PEEP_FLAGS_LITTER = (1 << 9),     // Makes the peep throw litter
    PEEP_FLAGS_LOST = (1 << 10),      // Makes the peep feel lost (animation triggered)
    PEEP_FLAGS_HUNGER = (1 << 11),    // Makes the peep become hungry quicker
    PEEP_FLAGS_BATHROOM = (1 << 12),  // Makes the peep want to go to the bathroom
    PEEP_FLAGS_CROWDED = (1 << 13),   // The peep will start feeling crowded
    PEEP_FLAGS_HAPPINESS = (1 << 14), // The peep will start increasing happiness
    PEEP_FLAGS_NAUSEA = (1 << 15),    // Makes the peep feel sick (e.g. after an extreme ride)
    PEEP_FLAGS_PURPLE = (1 << 16),    // Makes surrounding peeps purple
    PEEP_FLAGS_PIZZA = (1 << 17),     // Gives passing peeps pizza
    PEEP_FLAGS_EXPLODE = (1 << 18),
    PEEP_FLAGS_RIDE_SHOULD_BE_MARKED_AS_FAVOURITE = (1 << 19),
    PEEP_FLAGS_PARK_ENTRANCE_CHOSEN = (1 << 20), // Set when the nearest park entrance has been chosen
    PEEP_FLAGS_21 = (1 << 21),
    PEEP_FLAGS_CONTAGIOUS = (1 << 22), // Makes any peeps in surrounding tiles sick
    PEEP_FLAGS_JOY = (1 << 23),        // Makes the peep jump in joy
    PEEP_FLAGS_ANGRY = (1 << 24),
    PEEP_FLAGS_ICE_CREAM = (1 << 25),            // Gives passing peeps ice cream and they wave back
    PEEP_FLAGS_NICE_RIDE_DEPRECATED = (1 << 26), // Used to make the peep think "Nice ride! But not as good as the Phoenix..."
                                                 // on exiting a ride
    PEEP_FLAGS_INTAMIN_DEPRECATED = (1 << 27),   // Used to make the peep think "I'm so excited - It's an Intamin ride!" while
                                                 // riding on a Intamin ride.
    PEEP_FLAGS_HERE_WE_ARE = (1 << 28),          // Makes the peep think  "...and here we are on X!" while riding a ride
    PEEP_FLAGS_TWITCH = (1u << 31),              // Added for twitch integration
};

enum PeepNextFlags
{
    PEEP_NEXT_FLAG_DIRECTION_MASK = 0x3,
    PEEP_NEXT_FLAG_IS_SLOPED = (1 << 2),
    PEEP_NEXT_FLAG_IS_SURFACE = (1 << 3),
    PEEP_NEXT_FLAG_UNUSED = (1 << 4),
};

enum PeepNauseaTolerance
{
    PEEP_NAUSEA_TOLERANCE_NONE,
    PEEP_NAUSEA_TOLERANCE_LOW,
    PEEP_NAUSEA_TOLERANCE_AVERAGE,
    PEEP_NAUSEA_TOLERANCE_HIGH,
};

enum PeepItem
{
    // item_standard_flags
    PEEP_ITEM_BALLOON = (1 << 0),
    PEEP_ITEM_TOY = (1 << 1),
    PEEP_ITEM_MAP = (1 << 2),
    PEEP_ITEM_PHOTO = (1 << 3),
    PEEP_ITEM_UMBRELLA = (1 << 4),
    PEEP_ITEM_DRINK = (1 << 5),
    PEEP_ITEM_BURGER = (1 << 6),
    PEEP_ITEM_CHIPS = (1 << 7),
    PEEP_ITEM_ICE_CREAM = (1 << 8),
    PEEP_ITEM_CANDYFLOSS = (1 << 9),
    PEEP_ITEM_EMPTY_CAN = (1 << 10),
    PEEP_ITEM_RUBBISH = (1 << 11),
    PEEP_ITEM_EMPTY_BURGER_BOX = (1 << 12),
    PEEP_ITEM_PIZZA = (1 << 13),
    PEEP_ITEM_VOUCHER = (1 << 14),
    PEEP_ITEM_POPCORN = (1 << 15),
    PEEP_ITEM_HOT_DOG = (1 << 16),
    PEEP_ITEM_TENTACLE = (1 << 17),
    PEEP_ITEM_HAT = (1 << 18),
    PEEP_ITEM_TOFFEE_APPLE = (1 << 19),
    PEEP_ITEM_TSHIRT = (1 << 20),
    PEEP_ITEM_DOUGHNUT = (1 << 21),
    PEEP_ITEM_COFFEE = (1 << 22),
    PEEP_ITEM_EMPTY_CUP = (1 << 23),
    PEEP_ITEM_CHICKEN = (1 << 24),
    PEEP_ITEM_LEMONADE = (1 << 25),
    PEEP_ITEM_EMPTY_BOX = (1 << 26),
    PEEP_ITEM_EMPTY_BOTTLE = (1 << 27),

    // item_extra_flags
    PEEP_ITEM_PHOTO2 = (1 << 0),
    PEEP_ITEM_PHOTO3 = (1 << 1),
    PEEP_ITEM_PHOTO4 = (1 << 2),
    PEEP_ITEM_PRETZEL = (1 << 3),
    PEEP_ITEM_CHOCOLATE = (1 << 4),
    PEEP_ITEM_ICED_TEA = (1 << 5),
    PEEP_ITEM_FUNNEL_CAKE = (1 << 6),
    PEEP_ITEM_SUNGLASSES = (1 << 7),
    PEEP_ITEM_BEEF_NOODLES = (1 << 8),
    PEEP_ITEM_FRIED_RICE_NOODLES = (1 << 9),
    PEEP_ITEM_WONTON_SOUP = (1 << 10),
    PEEP_ITEM_MEATBALL_SOUP = (1 << 11),
    PEEP_ITEM_FRUIT_JUICE = (1 << 12),
    PEEP_ITEM_SOYBEAN_MILK = (1 << 13),
    PEEP_ITEM_SU_JONGKWA = (1 << 14),
    PEEP_ITEM_SUB_SANDWICH = (1 << 15),
    PEEP_ITEM_COOKIE = (1 << 16),
    PEEP_ITEM_EMPTY_BOWL_RED = (1 << 17),
    PEEP_ITEM_EMPTY_DRINK_CARTON = (1 << 18),
    PEEP_ITEM_EMPTY_JUICE_CUP = (1 << 19),
    PEEP_ITEM_ROAST_SAUSAGE = (1 << 20),
    PEEP_ITEM_EMPTY_BOWL_BLUE = (1 << 21),
};

enum PeepSpriteType : uint8_t
{
    PEEP_SPRITE_TYPE_NORMAL = 0,
    PEEP_SPRITE_TYPE_HANDYMAN = 1,
    PEEP_SPRITE_TYPE_MECHANIC = 2,
    PEEP_SPRITE_TYPE_SECURITY = 3,
    PEEP_SPRITE_TYPE_ENTERTAINER_PANDA = 4,
    PEEP_SPRITE_TYPE_ENTERTAINER_TIGER = 5,
    PEEP_SPRITE_TYPE_ENTERTAINER_ELEPHANT = 6,
    PEEP_SPRITE_TYPE_ENTERTAINER_ROMAN = 7,
    PEEP_SPRITE_TYPE_ENTERTAINER_GORILLA = 8,
    PEEP_SPRITE_TYPE_ENTERTAINER_SNOWMAN = 9,
    PEEP_SPRITE_TYPE_ENTERTAINER_KNIGHT = 10,
    PEEP_SPRITE_TYPE_ENTERTAINER_ASTRONAUT = 11,
    PEEP_SPRITE_TYPE_ENTERTAINER_BANDIT = 12,
    PEEP_SPRITE_TYPE_ENTERTAINER_SHERIFF = 13,
    PEEP_SPRITE_TYPE_ENTERTAINER_PIRATE = 14,
    PEEP_SPRITE_TYPE_ICE_CREAM = 15,
    PEEP_SPRITE_TYPE_CHIPS = 16,
    PEEP_SPRITE_TYPE_BURGER = 17,
    PEEP_SPRITE_TYPE_DRINK = 18,
    PEEP_SPRITE_TYPE_BALLOON = 19,
    PEEP_SPRITE_TYPE_CANDYFLOSS = 20,
    PEEP_SPRITE_TYPE_UMBRELLA = 21,
    PEEP_SPRITE_TYPE_PIZZA = 22,
    PEEP_SPRITE_TYPE_SECURITY_ALT = 23,
    PEEP_SPRITE_TYPE_POPCORN = 24,
    PEEP_SPRITE_TYPE_ARMS_CROSSED = 25,
    PEEP_SPRITE_TYPE_HEAD_DOWN = 26,
    PEEP_SPRITE_TYPE_NAUSEOUS = 27,
    PEEP_SPRITE_TYPE_VERY_NAUSEOUS = 28,
    PEEP_SPRITE_TYPE_REQUIRE_BATHROOM = 29,
    PEEP_SPRITE_TYPE_HAT = 30,
    PEEP_SPRITE_TYPE_HOT_DOG = 31,
    PEEP_SPRITE_TYPE_TENTACLE = 32,
    PEEP_SPRITE_TYPE_TOFFEE_APPLE = 33,
    PEEP_SPRITE_TYPE_DOUGHNUT = 34,
    PEEP_SPRITE_TYPE_COFFEE = 35,
    PEEP_SPRITE_TYPE_CHICKEN = 36,
    PEEP_SPRITE_TYPE_LEMONADE = 37,
    PEEP_SPRITE_TYPE_WATCHING = 38,
    PEEP_SPRITE_TYPE_PRETZEL = 39,
    PEEP_SPRITE_TYPE_SUNGLASSES = 40,
    PEEP_SPRITE_TYPE_SU_JONGKWA = 41,
    PEEP_SPRITE_TYPE_JUICE = 42,
    PEEP_SPRITE_TYPE_FUNNEL_CAKE = 43,
    PEEP_SPRITE_TYPE_NOODLES = 44,
    PEEP_SPRITE_TYPE_SAUSAGE = 45,
    PEEP_SPRITE_TYPE_SOUP = 46,
    PEEP_SPRITE_TYPE_SANDWICH = 47,
    PEEP_SPRITE_TYPE_COUNT,

    PEEP_SPRITE_TYPE_INVALID = 255
};

// Flags used by peep->window_invalidate_flags
enum PeepInvalidate
{
    PEEP_INVALIDATE_PEEP_THOUGHTS = 1,
    PEEP_INVALIDATE_PEEP_STATS = 1 << 1,
    PEEP_INVALIDATE_PEEP_2 = 1 << 2,
    PEEP_INVALIDATE_PEEP_INVENTORY = 1 << 3,
    PEEP_INVALIDATE_STAFF_STATS = 1 << 4,
    PEEP_INVALIDATE_PEEP_ACTION = 1 << 5, // Currently set only when guest_heading_to_ride_id is changed
};

// Flags used by peep_should_go_on_ride()
enum PeepRideDecision
{
    PEEP_RIDE_DECISION_AT_QUEUE = 1,
    PEEP_RIDE_DECISION_THINKING = 1 << 2,
};

struct rct_peep_thought
{
    PeepThoughtType type;  // 0
    uint8_t item;          // 1
    uint8_t freshness;     // 2 larger is less fresh
    uint8_t fresh_timeout; // 3 updates every tick
};

struct Guest;
struct Staff;

struct Peep : SpriteBase
{
    char* name;
    CoordsXYZ NextLoc;
    uint8_t next_flags;
    uint8_t outside_of_park;
    PeepState state;
    uint8_t sub_state;
    PeepSpriteType sprite_type;
    PeepType type;
    union
    {
        uint8_t staff_type;
        uint8_t no_of_rides;
    };
    uint8_t tshirt_colour;
    uint8_t trousers_colour;
    uint16_t destination_x; // Location that the peep is trying to get to
    uint16_t destination_y;
    uint8_t destination_tolerance; // How close to destination before next action/state 0 = exact
    uint8_t var_37;
    uint8_t energy;
    uint8_t energy_target;
    uint8_t happiness;
    uint8_t happiness_target;
    uint8_t nausea;
    uint8_t nausea_target;
    uint8_t hunger;
    uint8_t thirst;
    uint8_t toilet;
    uint8_t mass;
    uint8_t time_to_consume;
    uint8_t intensity; // The max intensity is stored in the first 4 bits, and the min intensity in the second 4 bits
    uint8_t nausea_tolerance;
    uint8_t window_invalidate_flags;
    money16 paid_on_drink;
    uint8_t ride_types_been_on[16];
    uint32_t item_extra_flags;
    uint8_t photo2_ride_ref;
    uint8_t photo3_ride_ref;
    uint8_t photo4_ride_ref;
    uint8_t current_ride;
    StationIndex current_ride_station;
    uint8_t current_train;
    union
    {
        struct
        {
            uint8_t current_car;
            uint8_t current_seat;
        };
        uint16_t time_to_sitdown;
        struct
        {
            uint8_t time_to_stand;
            uint8_t standing_flags;
        };
    };
    // Normally 0, 1 for carrying sliding board on spiral slide ride, 2 for carrying lawn mower
    uint8_t special_sprite;
    PeepActionSpriteType action_sprite_type;
    // Seems to be used like a local variable, as it's always set before calling SwitchNextActionSpriteType, which
    // reads this again
    PeepActionSpriteType next_action_sprite_type;
    uint8_t action_sprite_image_offset;
    PeepActionType action;
    uint8_t action_frame;
    uint8_t step_progress;
    union
    {
        uint16_t mechanic_time_since_call; // time getting to ride to fix
        uint16_t next_in_queue;
    };
    union
    {
        uint8_t maze_last_edge;
        Direction direction; // Direction ?
    };
    uint8_t interaction_ride_index;
    uint16_t time_in_queue;
    uint8_t rides_been_on[32];
    // 255 bit bitmap of every ride the peep has been on see
    // window_peep_rides_update for how to use.
    uint32_t id;
    money32 cash_in_pocket;
    money32 cash_spent;
    int32_t time_in_park;
    int8_t rejoin_queue_timeout; // whilst waiting for a free vehicle (or pair) in the entrance
    uint8_t previous_ride;
    uint16_t previous_ride_time_out;
    rct_peep_thought thoughts[PEEP_MAX_THOUGHTS];
    uint8_t path_check_optimisation; // see peep.checkForPath
    union
    {
        uint8_t staff_id;
        uint8_t guest_heading_to_ride_id;
    };
    union
    {
        uint8_t staff_orders;
        uint8_t peep_is_lost_countdown;
    };
    uint8_t photo1_ride_ref;
    uint32_t peep_flags;
    rct12_xyzd8 pathfind_goal;
    rct12_xyzd8 pathfind_history[4];
    uint8_t no_action_frame_num;
    // 0x3F Litter Count split into lots of 3 with time, 0xC0 Time since last recalc
    uint8_t litter_count;
    union
    {
        uint8_t time_on_ride;
        uint8_t staff_mowing_timeout;
    };
    // 0x3F Sick Count split into lots of 3 with time, 0xC0 Time since last recalc
    uint8_t disgusting_count;
    union
    {
        money16 paid_to_enter;
        uint16_t staff_lawns_mown;
        uint16_t staff_rides_fixed;
    };
    union
    {
        money16 paid_on_rides;
        uint16_t staff_gardens_watered;
        uint16_t staff_rides_inspected;
    };
    union
    {
        money16 paid_on_food;
        uint16_t staff_litter_swept;
    };
    union
    {
        money16 paid_on_souvenirs;
        uint16_t staff_bins_emptied;
    };
    uint8_t no_of_food;
    uint8_t no_of_drinks;
    uint8_t no_of_souvenirs;
    uint8_t vandalism_seen; // 0xC0 vandalism thought timeout, 0x3F vandalism tiles seen
    uint8_t voucher_type;
    uint8_t voucher_arguments; // ride_id or string_offset_id
    uint8_t surroundings_thought_timeout;
    uint8_t angriness;
    uint8_t time_lost; // the time the peep has been lost when it reaches 254 generates the lost thought
    uint8_t days_in_queue;
    uint8_t balloon_colour;
    uint8_t umbrella_colour;
    uint8_t hat_colour;
    uint8_t favourite_ride;
    uint8_t favourite_ride_rating;
    uint32_t item_standard_flags;

public: // Peep
    Guest* AsGuest();
    Staff* AsStaff();

    void Update();
    std::optional<CoordsXY> UpdateAction(int16_t& xy_distance);
    std::optional<CoordsXY> UpdateAction();
    void SetState(PeepState new_state);
    void Remove();
    void Invalidate();
    void UpdateCurrentActionSpriteType();
    void SwitchToSpecialSprite(uint8_t special_sprite_id);
    void StateReset();
    void MoveTo(int16_t destX, int16_t destY, int16_t destZ);
    uint8_t GetNextDirection() const;
    bool GetNextIsSloped() const;
    bool GetNextIsSurface() const;
    void SetNextFlags(uint8_t next_direction, bool is_sloped, bool is_surface);
    void Pickup();
    void PickupAbort(int32_t old_x);
    bool Place(const TileCoordsXYZ& location, bool apply);
    static Peep* Generate(const CoordsXYZ& coords);
    void RemoveFromQueue();
    void RemoveFromRide();
    void InsertNewThought(PeepThoughtType thought_type, uint8_t thought_arguments);
    void FormatActionTo(void* args) const;
    size_t FormatNameTo(void* args) const;
    std::string GetName() const;
    bool SetName(const std::string_view& value);

    // TODO: Make these private again when done refactoring
public: // Peep
    bool CheckForPath();
    void PerformNextAction(uint8_t& pathing_result);
    void PerformNextAction(uint8_t& pathing_result, TileElement*& tile_result);
    int32_t GetZOnSlope(int32_t tile_x, int32_t tile_y);
    void SwitchNextActionSpriteType();
    PeepActionSpriteType GetActionSpriteType();

private:
    void UpdateFalling();
    void Update1();
    void UpdatePicked();
};

struct Guest : Peep
{
public:
    void UpdateGuest();
    void Tick128UpdateGuest(int32_t index);
    bool HasItem(int32_t peepItem) const;
    bool HasFood() const;
    bool HasDrink() const;
    bool HasEmptyContainer() const;
    void OnEnterRide(ride_id_t rideIndex);
    void OnExitRide(ride_id_t rideIndex);
    void UpdateSpriteType();
    bool HeadingForRideOrParkExit() const;
    void StopPurchaseThought(uint8_t ride_type);
    void TryGetUpFromSitting();
    void ChoseNotToGoOnRide(Ride* ride, bool peepAtRide, bool updateLastRide);
    void PickRideToGoOn();
    void ReadMap();
    bool ShouldGoOnRide(Ride* ride, int32_t entranceNum, bool atQueue, bool thinking);
    bool ShouldGoToShop(Ride* ride, bool peepAtShop);
    bool ShouldFindBench();
    bool UpdateWalkingFindBench();
    bool UpdateWalkingFindBin();
    void SpendMoney(money16& peep_expend_type, money32 amount, ExpenditureType type);
    void SpendMoney(money32 amount, ExpenditureType type);
    void SetHasRidden(const Ride* ride);
    bool HasRidden(const Ride* ride) const;
    void SetHasRiddenRideType(int32_t rideType);
    bool HasRiddenRideType(int32_t rideType) const;
    int32_t HasFoodStandardFlag() const;
    int32_t HasFoodExtraFlag() const;
    bool HasDrinkStandardFlag() const;
    bool HasDrinkExtraFlag() const;
    int32_t HasEmptyContainerStandardFlag() const;
    int32_t HasEmptyContainerExtraFlag() const;
    void CheckIfLost();
    void CheckCantFindRide();
    void CheckCantFindExit();
    bool DecideAndBuyItem(Ride* ride, int32_t shopItem, money32 price);
    void SetSpriteType(PeepSpriteType new_sprite_type);
    void HandleEasterEggName();
    int32_t GetEasterEggNameId() const;
    void UpdateEasterEggInteractions();

private:
    void UpdateRide();
    void UpdateOnRide(){}; // TODO
    void UpdateWalking();
    void UpdateQueuing();
    void UpdateSitting();
    void UpdateEnteringPark();
    void UpdateLeavingPark();
    void UpdateBuying();
    void UpdateWatching();
    void UpdateUsingBin();
    void UpdateRideAtEntrance();
    void UpdateRideAdvanceThroughEntrance();
    void UpdateRideFreeVehicleCheck();
    void UpdateRideFreeVehicleEnterRide(Ride* ride);
    void UpdateRideApproachVehicle();
    void UpdateRideEnterVehicle();
    void UpdateRideLeaveVehicle();
    void UpdateRideApproachExit();
    void UpdateRideInExit();
    void UpdateRideApproachVehicleWaypoints();
    void UpdateRideApproachExitWaypoints();
    void UpdateRideApproachSpiralSlide();
    void UpdateRideOnSpiralSlide();
    void UpdateRideLeaveSpiralSlide();
    void UpdateRideMazePathfinding();
    void UpdateRideLeaveExit();
    void UpdateRideShopApproach();
    void UpdateRideShopInteract();
    void UpdateRideShopLeave();
    using easter_egg_function = void (Guest::*)(Guest* otherGuest);
    int32_t CheckEasterEggName(int32_t index) const;
    void ApplyEasterEggToNearbyGuests(easter_egg_function easter_egg);
    bool GuestHasValidXY() const;
    void GivePassingPeepsPurpleClothes(Guest* passingPeep);
    void GivePassingPeepsPizza(Guest* passingPeep);
    void MakePassingPeepsSick(Guest* passingPeep);
    void GivePassingPeepsIceCream(Guest* passingPeep);
    Ride* FindBestRideToGoOn();
    std::bitset<MAX_RIDES> FindRidesToGoOn();
    bool FindVehicleToEnter(Ride* ride, std::vector<uint8_t>& car_array);
    void GoToRideEntrance(Ride* ride);
};

struct Staff : Peep
{
public:
    void UpdateStaff(uint32_t stepsToTake);
    void Tick128UpdateStaff();
    bool IsMechanic() const;
    bool IsPatrolAreaSet(const CoordsXY& coords) const;
    bool IsLocationInPatrol(const CoordsXY& loc) const;
    bool DoPathFinding();

private:
    void UpdatePatrolling();
    void UpdateMowing();
    void UpdateSweeping();
    void UpdateEmptyingBin();
    void UpdateWatering();
    void UpdateAnswering();
    void UpdateFixing(int32_t steps);
    bool UpdateFixingEnterStation(Ride* ride);
    bool UpdateFixingMoveToBrokenDownVehicle(bool firstRun, Ride* ride);
    bool UpdateFixingFixVehicle(bool firstRun, Ride* ride);
    bool UpdateFixingFixVehicleMalfunction(bool firstRun, Ride* ride);
    bool UpdateFixingMoveToStationEnd(bool firstRun, Ride* ride);
    bool UpdateFixingFixStationEnd(bool firstRun);
    bool UpdateFixingMoveToStationStart(bool firstRun, Ride* ride);
    bool UpdateFixingFixStationStart(bool firstRun, Ride* ride);
    bool UpdateFixingFixStationBrakes(bool firstRun, Ride* ride);
    bool UpdateFixingMoveToStationExit(bool firstRun, Ride* ride);
    bool UpdateFixingFinishFixOrInspect(bool firstRun, int32_t steps, Ride* ride);
    bool UpdateFixingLeaveByEntranceExit(bool firstRun, Ride* ride);
    void UpdateRideInspected(ride_id_t rideIndex);
    void UpdateHeadingToInspect();

    bool DoHandymanPathFinding();
    bool DoMechanicPathFinding();
    bool DoEntertainerPathFinding();
    bool DoMiscPathFinding();

    int32_t HandymanDirectionRandSurface(uint8_t validDirections);
};

static_assert(sizeof(Peep) <= 512);

struct rct_sprite_bounds
{
    uint8_t sprite_width;           // 0x00
    uint8_t sprite_height_negative; // 0x01
    uint8_t sprite_height_positive; // 0x02
};

struct rct_peep_animation
{
    uint32_t base_image; // 0x00
    size_t num_frames;
    const uint8_t* frame_offsets;
};

struct rct_peep_animation_entry
{
    const rct_peep_animation* sprite_animation; // 0x00
    const rct_sprite_bounds* sprite_bounds;     // 0x04
};

enum
{
    EASTEREGG_PEEP_NAME_MICHAEL_SCHUMACHER,
    EASTEREGG_PEEP_NAME_JACQUES_VILLENEUVE,
    EASTEREGG_PEEP_NAME_DAMON_HILL,
    EASTEREGG_PEEP_NAME_MR_BEAN,
    EASTEREGG_PEEP_NAME_CHRIS_SAWYER,
    EASTEREGG_PEEP_NAME_KATIE_BRAYSHAW,
    EASTEREGG_PEEP_NAME_MELANIE_WARN,
    EASTEREGG_PEEP_NAME_SIMON_FOSTER,
    EASTEREGG_PEEP_NAME_JOHN_WARDLEY,
    EASTEREGG_PEEP_NAME_LISA_STIRLING,
    EASTEREGG_PEEP_NAME_DONALD_MACRAE,
    EASTEREGG_PEEP_NAME_KATHERINE_MCGOWAN,
    EASTEREGG_PEEP_NAME_FRANCES_MCGOWAN,
    EASTEREGG_PEEP_NAME_CORINA_MASSOURA,
    EASTEREGG_PEEP_NAME_CAROL_YOUNG,
    EASTEREGG_PEEP_NAME_MIA_SHERIDAN,
    EASTEREGG_PEEP_NAME_KATIE_RODGER,
    EASTEREGG_PEEP_NAME_EMMA_GARRELL,
    EASTEREGG_PEEP_NAME_JOANNE_BARTON,
    EASTEREGG_PEEP_NAME_FELICITY_ANDERSON,
    EASTEREGG_PEEP_NAME_KATIE_SMITH,
    EASTEREGG_PEEP_NAME_EILIDH_BELL,
    EASTEREGG_PEEP_NAME_NANCY_STILLWAGON,
    EASTEREGG_PEEP_NAME_DAVID_ELLIS
};

enum
{
    PATHING_DESTINATION_REACHED = 1 << 0,
    PATHING_OUTSIDE_PARK = 1 << 1,
    PATHING_RIDE_EXIT = 1 << 2,
    PATHING_RIDE_ENTRANCE = 1 << 3,
};

/** Helper macro until rides are stored in this module. */
#define GET_PEEP(sprite_index) &(get_sprite(sprite_index)->peep)

/**
 * Helper macro loop for enumerating through all the peeps. To avoid needing a end loop counterpart, statements are
 * applied in tautology if statements.
 */
#define FOR_ALL_PEEPS(sprite_index, peep)                                                                                      \
    for ((sprite_index) = gSpriteListHead[SPRITE_LIST_PEEP]; (sprite_index) != SPRITE_INDEX_NULL; (sprite_index) = peep->next) \
        if (((peep) = GET_PEEP(sprite_index)) != nullptr || 1)

#define FOR_ALL_GUESTS(sprite_index, peep)                                                                                     \
    FOR_ALL_PEEPS (sprite_index, peep)                                                                                         \
        if ((peep)->type == PEEP_TYPE_GUEST)

#define FOR_ALL_STAFF(sprite_index, peep)                                                                                      \
    FOR_ALL_PEEPS (sprite_index, peep)                                                                                         \
        if ((peep)->type == PEEP_TYPE_STAFF)

// rct2: 0x00982708
extern rct_peep_animation_entry g_peep_animation_entries[PEEP_SPRITE_TYPE_COUNT];
extern const bool gSpriteTypeToSlowWalkMap[48];

extern uint8_t gGuestChangeModifier;
extern uint32_t gNumGuestsInPark;
extern uint32_t gNumGuestsInParkLastWeek;
extern uint32_t gNumGuestsHeadingForPark;

extern money16 gGuestInitialCash;
extern uint8_t gGuestInitialHappiness;
extern uint8_t gGuestInitialHunger;
extern uint8_t gGuestInitialThirst;

extern uint32_t gNextGuestNumber;

extern uint8_t gPeepWarningThrottle[16];

extern TileCoordsXYZ gPeepPathFindGoalPosition;
extern bool gPeepPathFindIgnoreForeignQueues;
extern ride_id_t gPeepPathFindQueueRideIndex;

Peep* try_get_guest(uint16_t spriteIndex);
int32_t peep_get_staff_count();
bool peep_can_be_picked_up(Peep* peep);
void peep_update_all();
void peep_problem_warnings_update();
void peep_stop_crowd_noise();
void peep_update_crowd_noise();
void peep_update_days_in_queue();
void peep_applause();
void peep_thought_set_format_args(const rct_peep_thought* thought);
int32_t get_peep_face_sprite_small(Peep* peep);
int32_t get_peep_face_sprite_large(Peep* peep);
void game_command_pickup_guest(
    int32_t* eax, int32_t* ebx, int32_t* ecx, int32_t* edx, int32_t* esi, int32_t* edi, int32_t* ebp);
void peep_sprite_remove(Peep* peep);

void peep_window_state_update(Peep* peep);
void peep_decrement_num_riders(Peep* peep);

void peep_set_map_tooltip(Peep* peep);
int32_t peep_compare(const void* sprite_index_a, const void* sprite_index_b);

void SwitchToSpecialSprite(Peep* peep, uint8_t special_sprite_id);
void peep_update_names(bool realNames);

void guest_set_name(uint16_t spriteIndex, const char* name);

Direction peep_pathfind_choose_direction(const TileCoordsXYZ& loc, Peep* peep);
void peep_reset_pathfind_goal(Peep* peep);

bool is_valid_path_z_and_direction(TileElement* tileElement, int32_t currentZ, int32_t currentDirection);
int32_t guest_path_finding(Guest* peep);

#if defined(DEBUG_LEVEL_1) && DEBUG_LEVEL_1
#    define PATHFIND_DEBUG                                                                                                     \
        0 // Set to 0 to disable pathfinding debugging;
          // Set to 1 to enable pathfinding debugging.
// Some variables used for the path finding debugging.
extern bool gPathFindDebug;              // Use to guard calls to log messages
extern utf8 gPathFindDebugPeepName[256]; // Use to put the peep name in the log message

// The following calls set the above two variables for a peep.
// ... when PATHFIND_DEBUG is 1 (nonzero)
void pathfind_logging_enable(Peep* peep);
void pathfind_logging_disable();
#endif // defined(DEBUG_LEVEL_1) && DEBUG_LEVEL_1

void increment_guests_in_park();
void increment_guests_heading_for_park();
void decrement_guests_in_park();
void decrement_guests_heading_for_park();

rct_string_id get_real_name_string_id_from_id(uint32_t id);

#endif
