/*****************************************************************************
 * Copyright (c) 2014-2019 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "MapAnimation.h"

#include "../Context.h"
#include "../Game.h"
#include "../interface/Viewport.h"
#include "../object/StationObject.h"
#include "../ride/Ride.h"
#include "../ride/RideData.h"
#include "../ride/Track.h"
#include "../world/Wall.h"
#include "Banner.h"
#include "Footpath.h"
#include "LargeScenery.h"
#include "Map.h"
#include "Scenery.h"
#include "SmallScenery.h"
#include "Sprite.h"

using map_animation_invalidate_event_handler = bool (*)(const CoordsXYZ& loc);

static std::vector<MapAnimation> _mapAnimations;

constexpr size_t MAX_ANIMATED_OBJECTS = 2000;

static bool InvalidateMapAnimation(const MapAnimation& obj);

static bool DoesAnimationExist(int32_t type, const CoordsXYZ& location)
{
    for (const auto& a : _mapAnimations)
    {
        if (a.type == type && a.location == location)
        {
            // Animation already exists
            return true;
        }
    }
    return false;
}

void map_animation_create(int32_t type, const CoordsXYZ& loc)
{
    if (!DoesAnimationExist(type, loc))
    {
        if (_mapAnimations.size() < MAX_ANIMATED_OBJECTS)
        {
            // Create new animation
            _mapAnimations.push_back({ (uint8_t)type, loc });
        }
        else
        {
            log_error("Exceeded the maximum number of animations");
        }
    }
}

/**
 *
 *  rct2: 0x0068AFAD
 */
void map_animation_invalidate_all()
{
    auto it = _mapAnimations.begin();
    while (it != _mapAnimations.end())
    {
        if (InvalidateMapAnimation(*it))
        {
            // Map animation has finished, remove it
            it = _mapAnimations.erase(it);
        }
        else
        {
            it++;
        }
    }
}

/**
 *
 *  rct2: 0x00666670
 */
static bool map_animation_invalidate_ride_entrance(const CoordsXYZ& loc)
{
    TileCoordsXYZ tileLoc{ loc };
    auto tileElement = map_get_first_element_at(loc);
    if (tileElement == nullptr)
        return true;
    do
    {
        if (tileElement->base_height != tileLoc.z)
            continue;
        if (tileElement->GetType() != TILE_ELEMENT_TYPE_ENTRANCE)
            continue;
        if (tileElement->AsEntrance()->GetEntranceType() != ENTRANCE_TYPE_RIDE_ENTRANCE)
            continue;

        auto ride = get_ride(tileElement->AsEntrance()->GetRideIndex());
        if (ride != nullptr)
        {
            auto stationObj = ride_get_station_object(ride);
            if (stationObj != nullptr)
            {
                int32_t height = loc.z + stationObj->Height + 8;
                map_invalidate_tile_zoom1({ loc, height, height + 16 });
            }
        }
        return false;
    } while (!(tileElement++)->IsLastForTile());

    return true;
}

/**
 *
 *  rct2: 0x006A7BD4
 */
static bool map_animation_invalidate_queue_banner(const CoordsXYZ& loc)
{
    TileCoordsXYZ tileLoc{ loc };
    TileElement* tileElement;

    tileElement = map_get_first_element_at(loc);
    if (tileElement == nullptr)
        return true;
    do
    {
        if (tileElement->base_height != tileLoc.z)
            continue;
        if (tileElement->GetType() != TILE_ELEMENT_TYPE_PATH)
            continue;
        if (!(tileElement->AsPath()->IsQueue()))
            continue;
        if (!tileElement->AsPath()->HasQueueBanner())
            continue;

        int32_t direction = (tileElement->AsPath()->GetQueueBannerDirection() + get_current_rotation()) & 3;
        if (direction == TILE_ELEMENT_DIRECTION_NORTH || direction == TILE_ELEMENT_DIRECTION_EAST)
        {
            map_invalidate_tile_zoom1({ loc, loc.z + 16, loc.z + 30 });
        }
        return false;
    } while (!(tileElement++)->IsLastForTile());

    return true;
}

/**
 *
 *  rct2: 0x006E32C9
 */
static bool map_animation_invalidate_small_scenery(const CoordsXYZ& loc)
{
    TileCoordsXYZ tileLoc{ loc };
    TileElement* tileElement;
    rct_scenery_entry* sceneryEntry;
    rct_sprite* sprite;
    Peep* peep;

    tileElement = map_get_first_element_at(loc);
    if (tileElement == nullptr)
        return true;
    do
    {
        if (tileElement->base_height != tileLoc.z)
            continue;
        if (tileElement->GetType() != TILE_ELEMENT_TYPE_SMALL_SCENERY)
            continue;
        if (tileElement->IsGhost())
            continue;

        sceneryEntry = tileElement->AsSmallScenery()->GetEntry();
        if (sceneryEntry == nullptr)
            continue;

        if (scenery_small_entry_has_flag(
                sceneryEntry,
                SMALL_SCENERY_FLAG_FOUNTAIN_SPRAY_1 | SMALL_SCENERY_FLAG_FOUNTAIN_SPRAY_4 | SMALL_SCENERY_FLAG_SWAMP_GOO
                    | SMALL_SCENERY_FLAG_HAS_FRAME_OFFSETS))
        {
            map_invalidate_tile_zoom1({ loc, loc.z, tileElement->GetClearanceZ() });
            return false;
        }

        if (scenery_small_entry_has_flag(sceneryEntry, SMALL_SCENERY_FLAG_IS_CLOCK))
        {
            // Peep, looking at scenery
            if (!(gCurrentTicks & 0x3FF) && game_is_not_paused())
            {
                int32_t direction = tileElement->GetDirection();
                int32_t x2 = loc.x - CoordsDirectionDelta[direction].x;
                int32_t y2 = loc.y - CoordsDirectionDelta[direction].y;

                uint16_t spriteIdx = sprite_get_first_in_quadrant(x2, y2);
                for (; spriteIdx != SPRITE_INDEX_NULL; spriteIdx = sprite->generic.next_in_quadrant)
                {
                    sprite = get_sprite(spriteIdx);
                    if (sprite->generic.linked_list_index != SPRITE_LIST_PEEP)
                        continue;

                    peep = &sprite->peep;
                    if (peep->state != PEEP_STATE_WALKING)
                        continue;
                    if (peep->z != loc.z)
                        continue;
                    if (peep->action < PEEP_ACTION_NONE_1)
                        continue;

                    peep->action = PEEP_ACTION_CHECK_TIME;
                    peep->action_frame = 0;
                    peep->action_sprite_image_offset = 0;
                    peep->UpdateCurrentActionSpriteType();
                    invalidate_sprite_1(peep);
                    break;
                }
            }
            map_invalidate_tile_zoom1({ loc, loc.z, tileElement->GetClearanceZ() });
            return false;
        }

    } while (!(tileElement++)->IsLastForTile());
    return true;
}

/**
 *
 *  rct2: 0x00666C63
 */
static bool map_animation_invalidate_park_entrance(const CoordsXYZ& loc)
{
    TileCoordsXYZ tileLoc{ loc };
    TileElement* tileElement;

    tileElement = map_get_first_element_at(loc);
    if (tileElement == nullptr)
        return true;
    do
    {
        if (tileElement->base_height != tileLoc.z)
            continue;
        if (tileElement->GetType() != TILE_ELEMENT_TYPE_ENTRANCE)
            continue;
        if (tileElement->AsEntrance()->GetEntranceType() != ENTRANCE_TYPE_PARK_ENTRANCE)
            continue;
        if (tileElement->AsEntrance()->GetSequenceIndex())
            continue;

        map_invalidate_tile_zoom1({ loc, loc.z + 32, loc.z + 64 });
        return false;
    } while (!(tileElement++)->IsLastForTile());

    return true;
}

/**
 *
 *  rct2: 0x006CE29E
 */
static bool map_animation_invalidate_track_waterfall(const CoordsXYZ& loc)
{
    TileCoordsXYZ tileLoc{ loc };
    TileElement* tileElement;

    tileElement = map_get_first_element_at(loc);
    if (tileElement == nullptr)
        return true;
    do
    {
        if (tileElement->base_height != tileLoc.z)
            continue;
        if (tileElement->GetType() != TILE_ELEMENT_TYPE_TRACK)
            continue;

        if (tileElement->AsTrack()->GetTrackType() == TRACK_ELEM_WATERFALL)
        {
            map_invalidate_tile_zoom1({ loc, loc.z + 14, loc.z + 46 });
            return false;
        }
    } while (!(tileElement++)->IsLastForTile());

    return true;
}

/**
 *
 *  rct2: 0x006CE2F3
 */
static bool map_animation_invalidate_track_rapids(const CoordsXYZ& loc)
{
    TileCoordsXYZ tileLoc{ loc };
    TileElement* tileElement;

    tileElement = map_get_first_element_at(loc);
    if (tileElement == nullptr)
        return true;
    do
    {
        if (tileElement->base_height != tileLoc.z)
            continue;
        if (tileElement->GetType() != TILE_ELEMENT_TYPE_TRACK)
            continue;

        if (tileElement->AsTrack()->GetTrackType() == TRACK_ELEM_RAPIDS)
        {
            map_invalidate_tile_zoom1({ loc, loc.z + 14, loc.z + 18 });
            return false;
        }
    } while (!(tileElement++)->IsLastForTile());

    return true;
}

/**
 *
 *  rct2: 0x006CE39D
 */
static bool map_animation_invalidate_track_onridephoto(const CoordsXYZ& loc)
{
    TileCoordsXYZ tileLoc{ loc };
    TileElement* tileElement;

    tileElement = map_get_first_element_at(loc);
    if (tileElement == nullptr)
        return true;
    do
    {
        if (tileElement->base_height != tileLoc.z)
            continue;
        if (tileElement->GetType() != TILE_ELEMENT_TYPE_TRACK)
            continue;

        if (tileElement->AsTrack()->GetTrackType() == TRACK_ELEM_ON_RIDE_PHOTO)
        {
            map_invalidate_tile_zoom1({ loc, loc.z, tileElement->GetClearanceZ() });
            if (game_is_paused())
            {
                return false;
            }
            if (tileElement->AsTrack()->IsTakingPhoto())
            {
                tileElement->AsTrack()->DecrementPhotoTimeout();
                return false;
            }
            else
            {
                return true;
            }
        }
    } while (!(tileElement++)->IsLastForTile());

    return true;
}

/**
 *
 *  rct2: 0x006CE348
 */
static bool map_animation_invalidate_track_whirlpool(const CoordsXYZ& loc)
{
    TileCoordsXYZ tileLoc{ loc };
    TileElement* tileElement;

    tileElement = map_get_first_element_at(loc);
    if (tileElement == nullptr)
        return true;
    do
    {
        if (tileElement->base_height != tileLoc.z)
            continue;
        if (tileElement->GetType() != TILE_ELEMENT_TYPE_TRACK)
            continue;

        if (tileElement->AsTrack()->GetTrackType() == TRACK_ELEM_WHIRLPOOL)
        {
            map_invalidate_tile_zoom1({ loc, loc.z + 14, loc.z + 18 });
            return false;
        }
    } while (!(tileElement++)->IsLastForTile());

    return true;
}

/**
 *
 *  rct2: 0x006CE3FA
 */
static bool map_animation_invalidate_track_spinningtunnel(const CoordsXYZ& loc)
{
    TileCoordsXYZ tileLoc{ loc };
    TileElement* tileElement;

    tileElement = map_get_first_element_at(loc);
    if (tileElement == nullptr)
        return true;
    do
    {
        if (tileElement->base_height != tileLoc.z)
            continue;
        if (tileElement->GetType() != TILE_ELEMENT_TYPE_TRACK)
            continue;

        if (tileElement->AsTrack()->GetTrackType() == TRACK_ELEM_SPINNING_TUNNEL)
        {
            map_invalidate_tile_zoom1({ loc, loc.z + 14, loc.z + 32 });
            return false;
        }
    } while (!(tileElement++)->IsLastForTile());

    return true;
}

/**
 *
 *  rct2: 0x0068DF8F
 */
static bool map_animation_invalidate_remove([[maybe_unused]] const CoordsXYZ& loc)
{
    return true;
}

/**
 *
 *  rct2: 0x006BA2BB
 */
static bool map_animation_invalidate_banner(const CoordsXYZ& loc)
{
    TileCoordsXYZ tileLoc{ loc };
    TileElement* tileElement;

    tileElement = map_get_first_element_at(loc);
    if (tileElement == nullptr)
        return true;
    do
    {
        if (tileElement->base_height != tileLoc.z)
            continue;
        if (tileElement->GetType() != TILE_ELEMENT_TYPE_BANNER)
            continue;
        map_invalidate_tile_zoom1({ loc, loc.z, loc.z + 16 });
        return false;
    } while (!(tileElement++)->IsLastForTile());

    return true;
}

/**
 *
 *  rct2: 0x006B94EB
 */
static bool map_animation_invalidate_large_scenery(const CoordsXYZ& loc)
{
    TileCoordsXYZ tileLoc{ loc };
    TileElement* tileElement;
    rct_scenery_entry* sceneryEntry;

    bool wasInvalidated = false;
    tileElement = map_get_first_element_at(loc);
    if (tileElement == nullptr)
        return true;
    do
    {
        if (tileElement->base_height != tileLoc.z)
            continue;
        if (tileElement->GetType() != TILE_ELEMENT_TYPE_LARGE_SCENERY)
            continue;

        sceneryEntry = tileElement->AsLargeScenery()->GetEntry();
        if (sceneryEntry->large_scenery.flags & LARGE_SCENERY_FLAG_ANIMATED)
        {
            map_invalidate_tile_zoom1({ loc, loc.z, loc.z + 16 });
            wasInvalidated = true;
        }
    } while (!(tileElement++)->IsLastForTile());

    return !wasInvalidated;
}

/**
 *
 *  rct2: 0x006E5B50
 */
static bool map_animation_invalidate_wall_door(const CoordsXYZ& loc)
{
    TileCoordsXYZ tileLoc{ loc };
    TileElement* tileElement;
    rct_scenery_entry* sceneryEntry;

    if (gCurrentTicks & 1)
        return false;

    bool removeAnimation = true;
    tileElement = map_get_first_element_at(loc);
    if (tileElement == nullptr)
        return removeAnimation;
    do
    {
        if (tileElement->base_height != tileLoc.z)
            continue;
        if (tileElement->GetType() != TILE_ELEMENT_TYPE_WALL)
            continue;

        sceneryEntry = tileElement->AsWall()->GetEntry();
        if (!(sceneryEntry->wall.flags & WALL_SCENERY_IS_DOOR))
            continue;

        if (game_is_paused())
        {
            return false;
        }

        bool invalidate = false;

        uint8_t currentFrame = tileElement->AsWall()->GetAnimationFrame();
        if (currentFrame != 0)
        {
            if (currentFrame == 15)
            {
                currentFrame = 0;
            }
            else
            {
                removeAnimation = false;
                if (currentFrame != 5)
                {
                    currentFrame++;
                    if (currentFrame == 13 && !(sceneryEntry->wall.flags & WALL_SCENERY_LONG_DOOR_ANIMATION))
                        currentFrame = 15;

                    invalidate = true;
                }
            }
        }
        tileElement->AsWall()->SetAnimationFrame(currentFrame);
        if (invalidate)
        {
            map_invalidate_tile_zoom1({ loc, loc.z, loc.z + 32 });
        }
    } while (!(tileElement++)->IsLastForTile());

    return removeAnimation;
}

/**
 *
 *  rct2: 0x006E5EE4
 */
static bool map_animation_invalidate_wall(const CoordsXYZ& loc)
{
    TileCoordsXYZ tileLoc{ loc };
    TileElement* tileElement;
    rct_scenery_entry* sceneryEntry;

    bool wasInvalidated = false;
    tileElement = map_get_first_element_at(loc);
    if (tileElement == nullptr)
        return true;
    do
    {
        if (tileElement->base_height != tileLoc.z)
            continue;
        if (tileElement->GetType() != TILE_ELEMENT_TYPE_WALL)
            continue;

        sceneryEntry = tileElement->AsWall()->GetEntry();

        if (!sceneryEntry
            || (!(sceneryEntry->wall.flags2 & WALL_SCENERY_2_ANIMATED)
                && sceneryEntry->wall.scrolling_mode == SCROLLING_MODE_NONE))
            continue;

        map_invalidate_tile_zoom1({ loc, loc.z, loc.z + 16 });
        wasInvalidated = true;
    } while (!(tileElement++)->IsLastForTile());

    return !wasInvalidated;
}

/**
 *
 *  rct2: 0x009819DC
 */
static constexpr const map_animation_invalidate_event_handler _animatedObjectEventHandlers[MAP_ANIMATION_TYPE_COUNT] = {
    map_animation_invalidate_ride_entrance,
    map_animation_invalidate_queue_banner,
    map_animation_invalidate_small_scenery,
    map_animation_invalidate_park_entrance,
    map_animation_invalidate_track_waterfall,
    map_animation_invalidate_track_rapids,
    map_animation_invalidate_track_onridephoto,
    map_animation_invalidate_track_whirlpool,
    map_animation_invalidate_track_spinningtunnel,
    map_animation_invalidate_remove,
    map_animation_invalidate_banner,
    map_animation_invalidate_large_scenery,
    map_animation_invalidate_wall_door,
    map_animation_invalidate_wall
};

/**
 * @returns true if the animation should be removed.
 */
static bool InvalidateMapAnimation(const MapAnimation& a)
{
    if (a.type < std::size(_animatedObjectEventHandlers))
    {
        return _animatedObjectEventHandlers[a.type](a.location);
    }
    return true;
}

const std::vector<MapAnimation>& GetMapAnimations()
{
    return _mapAnimations;
}

static void ClearMapAnimations()
{
    _mapAnimations.clear();
}

void AutoCreateMapAnimations()
{
    ClearMapAnimations();

    tile_element_iterator it;
    tile_element_iterator_begin(&it);
    while (tile_element_iterator_next(&it))
    {
        auto el = it.element;
        auto loc = CoordsXYZ{ TileCoordsXY(it.x, it.y).ToCoordsXY(), el->GetBaseZ() };
        switch (el->GetType())
        {
            case TILE_ELEMENT_TYPE_BANNER:
                map_animation_create(MAP_ANIMATION_TYPE_BANNER, loc);
                break;
            case TILE_ELEMENT_TYPE_WALL:
            {
                auto wallEl = el->AsWall();
                auto entry = wallEl->GetEntry();
                if (entry != nullptr
                    && ((entry->wall.flags2 & WALL_SCENERY_2_ANIMATED) || entry->wall.scrolling_mode != SCROLLING_MODE_NONE))
                {
                    map_animation_create(MAP_ANIMATION_TYPE_WALL, loc);
                }
                break;
            }
            case TILE_ELEMENT_TYPE_SMALL_SCENERY:
            {
                auto sceneryEl = el->AsSmallScenery();
                auto entry = sceneryEl->GetEntry();
                if (entry != nullptr && scenery_small_entry_has_flag(entry, SMALL_SCENERY_FLAG_ANIMATED))
                {
                    map_animation_create(MAP_ANIMATION_TYPE_SMALL_SCENERY, loc);
                }
                break;
            }
            case TILE_ELEMENT_TYPE_LARGE_SCENERY:
            {
                auto sceneryEl = el->AsLargeScenery();
                auto entry = sceneryEl->GetEntry();
                if (entry != nullptr && (entry->large_scenery.flags & LARGE_SCENERY_FLAG_ANIMATED))
                {
                    map_animation_create(MAP_ANIMATION_TYPE_LARGE_SCENERY, loc);
                }
                break;
            }
            case TILE_ELEMENT_TYPE_PATH:
            {
                auto path = el->AsPath();
                if (path->HasQueueBanner())
                {
                    map_animation_create(MAP_ANIMATION_TYPE_QUEUE_BANNER, loc);
                }
                break;
            }
            case TILE_ELEMENT_TYPE_ENTRANCE:
            {
                auto entrance = el->AsEntrance();
                switch (entrance->GetEntranceType())
                {
                    case ENTRANCE_TYPE_PARK_ENTRANCE:
                        if (entrance->GetSequenceIndex() == 0)
                        {
                            map_animation_create(MAP_ANIMATION_TYPE_PARK_ENTRANCE, loc);
                        }
                        break;
                    case ENTRANCE_TYPE_RIDE_ENTRANCE:
                        map_animation_create(MAP_ANIMATION_TYPE_RIDE_ENTRANCE, loc);
                        break;
                }
                break;
            }
            case TILE_ELEMENT_TYPE_TRACK:
            {
                auto track = el->AsTrack();
                switch (track->GetTrackType())
                {
                    case TRACK_ELEM_WATERFALL:
                        map_animation_create(MAP_ANIMATION_TYPE_TRACK_WATERFALL, loc);
                        break;
                    case TRACK_ELEM_RAPIDS:
                        map_animation_create(MAP_ANIMATION_TYPE_TRACK_RAPIDS, loc);
                        break;
                    case TRACK_ELEM_WHIRLPOOL:
                        map_animation_create(MAP_ANIMATION_TYPE_TRACK_WHIRLPOOL, loc);
                        break;
                    case TRACK_ELEM_SPINNING_TUNNEL:
                        map_animation_create(MAP_ANIMATION_TYPE_TRACK_SPINNINGTUNNEL, loc);
                        break;
                }
                break;
            }
        }
    }
}
