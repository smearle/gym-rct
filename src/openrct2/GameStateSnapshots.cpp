#include "GameStateSnapshots.h"

#include "core/CircularBuffer.h"
#include "peep/Peep.h"
#include "world/Sprite.h"

static constexpr size_t MaximumGameStateSnapshots = 32;
static constexpr uint32_t InvalidTick = 0xFFFFFFFF;

struct GameStateSnapshot_t
{
    GameStateSnapshot_t& operator=(GameStateSnapshot_t&& mv) noexcept
    {
        tick = mv.tick;
        storedSprites = std::move(mv.storedSprites);
        return *this;
    }

    uint32_t tick = InvalidTick;
    uint32_t srand0 = 0;

    MemoryStream storedSprites;
    MemoryStream parkParameters;

    void SerialiseSprites(rct_sprite* sprites, const size_t numSprites, bool saving)
    {
        const bool loading = !saving;

        storedSprites.SetPosition(0);
        DataSerialiser ds(saving, storedSprites);

        std::vector<uint32_t> indexTable;
        indexTable.reserve(numSprites);

        uint32_t numSavedSprites = 0;

        if (saving)
        {
            for (size_t i = 0; i < numSprites; i++)
            {
                if (sprites[i].generic.sprite_identifier == SPRITE_IDENTIFIER_NULL)
                    continue;
                indexTable.push_back((uint32_t)i);
            }
            numSavedSprites = (uint32_t)indexTable.size();
        }

        ds << numSavedSprites;

        if (loading)
        {
            indexTable.resize(numSavedSprites);
        }

        for (uint32_t i = 0; i < numSavedSprites; i++)
        {
            ds << indexTable[i];

            const uint32_t spriteIdx = indexTable[i];
            rct_sprite& sprite = sprites[spriteIdx];

            ds << sprite.generic.sprite_identifier;

            switch (sprite.generic.sprite_identifier)
            {
                case SPRITE_IDENTIFIER_VEHICLE:
                    ds << reinterpret_cast<uint8_t(&)[sizeof(Vehicle)]>(sprite.vehicle);
                    break;
                case SPRITE_IDENTIFIER_PEEP:
                    ds << reinterpret_cast<uint8_t(&)[sizeof(Peep)]>(sprite.peep);
                    break;
                case SPRITE_IDENTIFIER_LITTER:
                    ds << reinterpret_cast<uint8_t(&)[sizeof(Litter)]>(sprite.litter);
                    break;
                case SPRITE_IDENTIFIER_MISC:
                {
                    ds << sprite.generic.type;
                    switch (sprite.generic.type)
                    {
                        case SPRITE_MISC_MONEY_EFFECT:
                            ds << reinterpret_cast<uint8_t(&)[sizeof(MoneyEffect)]>(sprite.money_effect);
                            break;
                        case SPRITE_MISC_BALLOON:
                            ds << reinterpret_cast<uint8_t(&)[sizeof(Balloon)]>(sprite.balloon);
                            break;
                        case SPRITE_MISC_DUCK:
                            ds << reinterpret_cast<uint8_t(&)[sizeof(Duck)]>(sprite.duck);
                            break;
                        case SPRITE_MISC_JUMPING_FOUNTAIN_WATER:
                            ds << reinterpret_cast<uint8_t(&)[sizeof(JumpingFountain)]>(sprite.jumping_fountain);
                            break;
                        case SPRITE_MISC_STEAM_PARTICLE:
                            ds << reinterpret_cast<uint8_t(&)[sizeof(SteamParticle)]>(sprite.steam_particle);
                            break;
                    }
                }
                break;
            }
        }
    }
};

struct GameStateSnapshots : public IGameStateSnapshots
{
    virtual void Reset() override final
    {
        _snapshots.clear();
    }

    virtual GameStateSnapshot_t& CreateSnapshot() override final
    {
        auto snapshot = std::make_unique<GameStateSnapshot_t>();
        _snapshots.push_back(std::move(snapshot));

        return *_snapshots.back();
    }

    virtual void LinkSnapshot(GameStateSnapshot_t& snapshot, uint32_t tick, uint32_t srand0) override final
    {
        snapshot.tick = tick;
        snapshot.srand0 = srand0;
    }

    virtual void Capture(GameStateSnapshot_t& snapshot) override final
    {
        snapshot.SerialiseSprites(get_sprite(0), MAX_SPRITES, true);

        // log_info("Snapshot size: %u bytes", (uint32_t)snapshot.storedSprites.GetLength());
    }

    virtual const GameStateSnapshot_t* GetLinkedSnapshot(uint32_t tick) const override final
    {
        for (size_t i = 0; i < _snapshots.size(); i++)
        {
            if (_snapshots[i]->tick == tick)
                return _snapshots[i].get();
        }
        return nullptr;
    }

    virtual void SerialiseSnapshot(GameStateSnapshot_t& snapshot, DataSerialiser& ds) const override final
    {
        ds << snapshot.tick;
        ds << snapshot.srand0;
        ds << snapshot.storedSprites;
        ds << snapshot.parkParameters;
    }

    std::vector<rct_sprite> BuildSpriteList(GameStateSnapshot_t& snapshot) const
    {
        std::vector<rct_sprite> spriteList;
        spriteList.resize(MAX_SPRITES);

        for (auto& sprite : spriteList)
        {
            // By default they don't exist.
            sprite.generic.sprite_identifier = SPRITE_IDENTIFIER_NULL;
        }

        snapshot.SerialiseSprites(spriteList.data(), MAX_SPRITES, false);

        return spriteList;
    }

#define COMPARE_FIELD(struc, field)                                                                                            \
    if (std::memcmp(&spriteBase.field, &spriteCmp.field, sizeof(struc::field)) != 0)                                           \
    {                                                                                                                          \
        uint64_t valA = 0;                                                                                                     \
        uint64_t valB = 0;                                                                                                     \
        std::memcpy(&valA, &spriteBase.field, sizeof(struc::field));                                                           \
        std::memcpy(&valB, &spriteCmp.field, sizeof(struc::field));                                                            \
        uintptr_t offset = reinterpret_cast<uintptr_t>(&spriteBase.field) - reinterpret_cast<uintptr_t>(&spriteBase);          \
        changeData.diffs.push_back(                                                                                            \
            GameStateSpriteChange_t::Diff_t{ (size_t)offset, sizeof(struc::field), #struc, #field, valA, valB });              \
    }

    void CompareSpriteDataCommon(
        const SpriteBase& spriteBase, const SpriteBase& spriteCmp, GameStateSpriteChange_t& changeData) const
    {
        COMPARE_FIELD(SpriteBase, sprite_identifier);
        COMPARE_FIELD(SpriteBase, type);
        COMPARE_FIELD(SpriteBase, next_in_quadrant);
        COMPARE_FIELD(SpriteBase, next);
        COMPARE_FIELD(SpriteBase, previous);
        COMPARE_FIELD(SpriteBase, linked_list_index);
        COMPARE_FIELD(SpriteBase, sprite_index);
        COMPARE_FIELD(SpriteBase, flags);
        COMPARE_FIELD(SpriteBase, x);
        COMPARE_FIELD(SpriteBase, y);
        COMPARE_FIELD(SpriteBase, z);
        /* Only relevant for rendering, does not affect game state.
        COMPARE_FIELD(SpriteBase, sprite_width);
        COMPARE_FIELD(SpriteBase, sprite_height_negative);
        COMPARE_FIELD(SpriteBase, sprite_height_positive);
        COMPARE_FIELD(SpriteBase, sprite_left);
        COMPARE_FIELD(SpriteBase, sprite_top);
        COMPARE_FIELD(SpriteBase, sprite_right);
        COMPARE_FIELD(SpriteBase, sprite_bottom);
        */
        COMPARE_FIELD(SpriteBase, sprite_direction);
    }

    void CompareSpriteDataPeep(const Peep& spriteBase, const Peep& spriteCmp, GameStateSpriteChange_t& changeData) const
    {
        COMPARE_FIELD(Peep, NextLoc.x);
        COMPARE_FIELD(Peep, NextLoc.y);
        COMPARE_FIELD(Peep, NextLoc.z);
        COMPARE_FIELD(Peep, next_flags);
        COMPARE_FIELD(Peep, outside_of_park);
        COMPARE_FIELD(Peep, state);
        COMPARE_FIELD(Peep, sub_state);
        COMPARE_FIELD(Peep, sprite_type);
        COMPARE_FIELD(Peep, type);
        COMPARE_FIELD(Peep, no_of_rides);
        COMPARE_FIELD(Peep, tshirt_colour);
        COMPARE_FIELD(Peep, trousers_colour);
        COMPARE_FIELD(Peep, destination_x);
        COMPARE_FIELD(Peep, destination_y);
        COMPARE_FIELD(Peep, destination_tolerance);
        COMPARE_FIELD(Peep, var_37);
        COMPARE_FIELD(Peep, energy);
        COMPARE_FIELD(Peep, energy_target);
        COMPARE_FIELD(Peep, happiness);
        COMPARE_FIELD(Peep, happiness_target);
        COMPARE_FIELD(Peep, nausea);
        COMPARE_FIELD(Peep, nausea_target);
        COMPARE_FIELD(Peep, hunger);
        COMPARE_FIELD(Peep, thirst);
        COMPARE_FIELD(Peep, toilet);
        COMPARE_FIELD(Peep, mass);
        COMPARE_FIELD(Peep, time_to_consume);
        COMPARE_FIELD(Peep, intensity);
        COMPARE_FIELD(Peep, nausea_tolerance);
        COMPARE_FIELD(Peep, window_invalidate_flags);
        COMPARE_FIELD(Peep, paid_on_drink);
        for (int i = 0; i < PEEP_MAX_THOUGHTS; i++)
        {
            COMPARE_FIELD(Peep, ride_types_been_on[i]);
        }
        COMPARE_FIELD(Peep, item_extra_flags);
        COMPARE_FIELD(Peep, photo2_ride_ref);
        COMPARE_FIELD(Peep, photo3_ride_ref);
        COMPARE_FIELD(Peep, photo4_ride_ref);
        COMPARE_FIELD(Peep, current_ride);
        COMPARE_FIELD(Peep, current_ride_station);
        COMPARE_FIELD(Peep, current_train);
        COMPARE_FIELD(Peep, time_to_sitdown);
        COMPARE_FIELD(Peep, special_sprite);
        COMPARE_FIELD(Peep, action_sprite_type);
        COMPARE_FIELD(Peep, next_action_sprite_type);
        COMPARE_FIELD(Peep, action_sprite_image_offset);
        COMPARE_FIELD(Peep, action);
        COMPARE_FIELD(Peep, action_frame);
        COMPARE_FIELD(Peep, step_progress);
        COMPARE_FIELD(Peep, next_in_queue);
        COMPARE_FIELD(Peep, maze_last_edge);
        COMPARE_FIELD(Peep, interaction_ride_index);
        COMPARE_FIELD(Peep, time_in_queue);
        for (int i = 0; i < 32; i++)
        {
            COMPARE_FIELD(Peep, rides_been_on[i]);
        }
        COMPARE_FIELD(Peep, id);
        COMPARE_FIELD(Peep, cash_in_pocket);
        COMPARE_FIELD(Peep, cash_spent);
        COMPARE_FIELD(Peep, time_in_park);
        COMPARE_FIELD(Peep, rejoin_queue_timeout);
        COMPARE_FIELD(Peep, previous_ride);
        COMPARE_FIELD(Peep, previous_ride_time_out);
        for (int i = 0; i < PEEP_MAX_THOUGHTS; i++)
        {
            COMPARE_FIELD(Peep, thoughts[i]);
        }
        COMPARE_FIELD(Peep, path_check_optimisation);
        COMPARE_FIELD(Peep, guest_heading_to_ride_id);
        COMPARE_FIELD(Peep, staff_orders);
        COMPARE_FIELD(Peep, photo1_ride_ref);
        COMPARE_FIELD(Peep, peep_flags);
        COMPARE_FIELD(Peep, pathfind_goal);
        for (int i = 0; i < 4; i++)
        {
            COMPARE_FIELD(Peep, pathfind_history[i]);
        }
        COMPARE_FIELD(Peep, no_action_frame_num);
        COMPARE_FIELD(Peep, litter_count);
        COMPARE_FIELD(Peep, time_on_ride);
        COMPARE_FIELD(Peep, disgusting_count);
        COMPARE_FIELD(Peep, paid_to_enter);
        COMPARE_FIELD(Peep, paid_on_rides);
        COMPARE_FIELD(Peep, paid_on_food);
        COMPARE_FIELD(Peep, paid_on_souvenirs);
        COMPARE_FIELD(Peep, no_of_food);
        COMPARE_FIELD(Peep, no_of_drinks);
        COMPARE_FIELD(Peep, no_of_souvenirs);
        COMPARE_FIELD(Peep, vandalism_seen);
        COMPARE_FIELD(Peep, voucher_type);
        COMPARE_FIELD(Peep, voucher_arguments);
        COMPARE_FIELD(Peep, surroundings_thought_timeout);
        COMPARE_FIELD(Peep, angriness);
        COMPARE_FIELD(Peep, time_lost);
        COMPARE_FIELD(Peep, days_in_queue);
        COMPARE_FIELD(Peep, balloon_colour);
        COMPARE_FIELD(Peep, umbrella_colour);
        COMPARE_FIELD(Peep, hat_colour);
        COMPARE_FIELD(Peep, favourite_ride);
        COMPARE_FIELD(Peep, favourite_ride_rating);
        COMPARE_FIELD(Peep, item_standard_flags);
    }

    void CompareSpriteDataVehicle(
        const Vehicle& spriteBase, const Vehicle& spriteCmp, GameStateSpriteChange_t& changeData) const
    {
        COMPARE_FIELD(Vehicle, vehicle_sprite_type);
        COMPARE_FIELD(Vehicle, bank_rotation);
        COMPARE_FIELD(Vehicle, remaining_distance);
        COMPARE_FIELD(Vehicle, velocity);
        COMPARE_FIELD(Vehicle, acceleration);
        COMPARE_FIELD(Vehicle, ride);
        COMPARE_FIELD(Vehicle, vehicle_type);
        COMPARE_FIELD(Vehicle, colours);
        COMPARE_FIELD(Vehicle, track_progress);
        COMPARE_FIELD(Vehicle, track_direction);
        COMPARE_FIELD(Vehicle, TrackLocation.x);
        COMPARE_FIELD(Vehicle, TrackLocation.y);
        COMPARE_FIELD(Vehicle, TrackLocation.z);
        COMPARE_FIELD(Vehicle, next_vehicle_on_train);
        COMPARE_FIELD(Vehicle, prev_vehicle_on_ride);
        COMPARE_FIELD(Vehicle, next_vehicle_on_ride);
        COMPARE_FIELD(Vehicle, var_44);
        COMPARE_FIELD(Vehicle, mass);
        COMPARE_FIELD(Vehicle, update_flags);
        COMPARE_FIELD(Vehicle, swing_sprite);
        COMPARE_FIELD(Vehicle, current_station);
        COMPARE_FIELD(Vehicle, swinging_car_var_0);
        COMPARE_FIELD(Vehicle, var_4E);
        COMPARE_FIELD(Vehicle, status);
        COMPARE_FIELD(Vehicle, sub_state);
        for (int i = 0; i < 32; i++)
        {
            COMPARE_FIELD(Vehicle, peep[i]);
        }
        for (int i = 0; i < 32; i++)
        {
            COMPARE_FIELD(Vehicle, peep_tshirt_colours[i]);
        }
        COMPARE_FIELD(Vehicle, num_seats);
        COMPARE_FIELD(Vehicle, num_peeps);
        COMPARE_FIELD(Vehicle, next_free_seat);
        COMPARE_FIELD(Vehicle, restraints_position);
        COMPARE_FIELD(Vehicle, spin_speed);
        COMPARE_FIELD(Vehicle, sound2_flags);
        COMPARE_FIELD(Vehicle, spin_sprite);
        COMPARE_FIELD(Vehicle, sound1_id);
        COMPARE_FIELD(Vehicle, sound1_volume);
        COMPARE_FIELD(Vehicle, sound2_id);
        COMPARE_FIELD(Vehicle, sound2_volume);
        COMPARE_FIELD(Vehicle, sound_vector_factor);
        COMPARE_FIELD(Vehicle, cable_lift_target);
        COMPARE_FIELD(Vehicle, speed);
        COMPARE_FIELD(Vehicle, powered_acceleration);
        COMPARE_FIELD(Vehicle, var_C4);
        COMPARE_FIELD(Vehicle, animation_frame);
        for (int i = 0; i < 2; i++)
        {
            COMPARE_FIELD(Vehicle, pad_C6[i]);
        }
        COMPARE_FIELD(Vehicle, var_C8);
        COMPARE_FIELD(Vehicle, var_CA);
        COMPARE_FIELD(Vehicle, scream_sound_id);
        COMPARE_FIELD(Vehicle, TrackSubposition);
        COMPARE_FIELD(Vehicle, num_laps);
        COMPARE_FIELD(Vehicle, brake_speed);
        COMPARE_FIELD(Vehicle, lost_time_out);
        COMPARE_FIELD(Vehicle, vertical_drop_countdown);
        COMPARE_FIELD(Vehicle, var_D3);
        COMPARE_FIELD(Vehicle, mini_golf_current_animation);
        COMPARE_FIELD(Vehicle, mini_golf_flags);
        COMPARE_FIELD(Vehicle, ride_subtype);
        COMPARE_FIELD(Vehicle, colours_extended);
        COMPARE_FIELD(Vehicle, seat_rotation);
        COMPARE_FIELD(Vehicle, target_seat_rotation);
    }

    void CompareSpriteDataLitter(const Litter& spriteBase, const Litter& spriteCmp, GameStateSpriteChange_t& changeData) const
    {
        COMPARE_FIELD(Litter, creationTick);
    }

    void CompareSpriteData(const rct_sprite& spriteBase, const rct_sprite& spriteCmp, GameStateSpriteChange_t& changeData) const
    {
        CompareSpriteDataCommon(spriteBase.generic, spriteCmp.generic, changeData);
        if (spriteBase.generic.sprite_identifier == spriteCmp.generic.sprite_identifier)
        {
            switch (spriteBase.generic.sprite_identifier)
            {
                case SPRITE_IDENTIFIER_PEEP:
                    CompareSpriteDataPeep(spriteBase.peep, spriteCmp.peep, changeData);
                    break;
                case SPRITE_IDENTIFIER_VEHICLE:
                    CompareSpriteDataVehicle(spriteBase.vehicle, spriteCmp.vehicle, changeData);
                    break;
                case SPRITE_IDENTIFIER_LITTER:
                    CompareSpriteDataLitter(spriteBase.litter, spriteCmp.litter, changeData);
                    break;
            }
        }
    }

    virtual GameStateCompareData_t Compare(const GameStateSnapshot_t& base, const GameStateSnapshot_t& cmp) const override final
    {
        GameStateCompareData_t res;
        res.tick = base.tick;
        res.srand0Left = base.srand0;
        res.srand0Right = cmp.srand0;

        std::vector<rct_sprite> spritesBase = BuildSpriteList(const_cast<GameStateSnapshot_t&>(base));
        std::vector<rct_sprite> spritesCmp = BuildSpriteList(const_cast<GameStateSnapshot_t&>(cmp));

        for (uint32_t i = 0; i < (uint32_t)spritesBase.size(); i++)
        {
            GameStateSpriteChange_t changeData;
            changeData.spriteIndex = i;

            const rct_sprite& spriteBase = spritesBase[i];
            const rct_sprite& spriteCmp = spritesCmp[i];

            changeData.spriteIdentifier = spriteBase.generic.sprite_identifier;
            changeData.miscIdentifier = spriteBase.generic.type;

            if (spriteBase.generic.sprite_identifier == SPRITE_IDENTIFIER_NULL
                && spriteCmp.generic.sprite_identifier != SPRITE_IDENTIFIER_NULL)
            {
                // Sprite was added.
                changeData.changeType = GameStateSpriteChange_t::ADDED;
                changeData.spriteIdentifier = spriteCmp.generic.sprite_identifier;
            }
            else if (
                spriteBase.generic.sprite_identifier != SPRITE_IDENTIFIER_NULL
                && spriteCmp.generic.sprite_identifier == SPRITE_IDENTIFIER_NULL)
            {
                // Sprite was removed.
                changeData.changeType = GameStateSpriteChange_t::REMOVED;
                changeData.spriteIdentifier = spriteBase.generic.sprite_identifier;
            }
            else if (
                spriteBase.generic.sprite_identifier == SPRITE_IDENTIFIER_NULL
                && spriteCmp.generic.sprite_identifier == SPRITE_IDENTIFIER_NULL)
            {
                // Do nothing.
                changeData.changeType = GameStateSpriteChange_t::EQUAL;
            }
            else
            {
                CompareSpriteData(spriteBase, spriteCmp, changeData);
                if (changeData.diffs.size() == 0)
                {
                    changeData.changeType = GameStateSpriteChange_t::EQUAL;
                }
                else
                {
                    changeData.changeType = GameStateSpriteChange_t::MODIFIED;
                }
            }

            res.spriteChanges.push_back(changeData);
        }

        return res;
    }

    static const char* GetSpriteIdentifierName(uint32_t spriteIdentifier, uint8_t miscIdentifier)
    {
        switch (spriteIdentifier)
        {
            case SPRITE_IDENTIFIER_NULL:
                return "Null";
            case SPRITE_IDENTIFIER_PEEP:
                return "Peep";
            case SPRITE_IDENTIFIER_VEHICLE:
                return "Vehicle";
            case SPRITE_IDENTIFIER_LITTER:
                return "Litter";
            case SPRITE_IDENTIFIER_MISC:
                switch (miscIdentifier)
                {
                    case SPRITE_MISC_STEAM_PARTICLE:
                        return "Misc: Steam Particle";
                    case SPRITE_MISC_MONEY_EFFECT:
                        return "Misc: Money effect";
                    case SPRITE_MISC_CRASHED_VEHICLE_PARTICLE:
                        return "Misc: Crash Vehicle Particle";
                    case SPRITE_MISC_EXPLOSION_CLOUD:
                        return "Misc: Explosion Cloud";
                    case SPRITE_MISC_CRASH_SPLASH:
                        return "Misc: Crash Splash";
                    case SPRITE_MISC_EXPLOSION_FLARE:
                        return "Misc: Explosion Flare";
                    case SPRITE_MISC_JUMPING_FOUNTAIN_WATER:
                        return "Misc: Jumping fountain water";
                    case SPRITE_MISC_BALLOON:
                        return "Misc: Balloon";
                    case SPRITE_MISC_DUCK:
                        return "Misc: Duck";
                    case SPRITE_MISC_JUMPING_FOUNTAIN_SNOW:
                        return "Misc: Jumping fountain snow";
                }
                return "Misc";
        }
        return "Unknown";
    }

    virtual bool LogCompareDataToFile(const std::string& fileName, const GameStateCompareData_t& cmpData) const override
    {
        std::string outputBuffer;
        char tempBuffer[1024] = {};

        snprintf(tempBuffer, sizeof(tempBuffer), "tick: %08X\n", cmpData.tick);
        outputBuffer += tempBuffer;

        snprintf(
            tempBuffer, sizeof(tempBuffer), "srand0 left = %08X, srand0 right = %08X\n", cmpData.srand0Left,
            cmpData.srand0Right);
        outputBuffer += tempBuffer;

        for (auto& change : cmpData.spriteChanges)
        {
            if (change.changeType == GameStateSpriteChange_t::EQUAL)
                continue;

            const char* typeName = GetSpriteIdentifierName(change.spriteIdentifier, change.miscIdentifier);

            if (change.changeType == GameStateSpriteChange_t::ADDED)
            {
                snprintf(tempBuffer, sizeof(tempBuffer), "Sprite added (%s), index: %u\n", typeName, change.spriteIndex);
                outputBuffer += tempBuffer;
            }
            else if (change.changeType == GameStateSpriteChange_t::REMOVED)
            {
                snprintf(tempBuffer, sizeof(tempBuffer), "Sprite removed (%s), index: %u\n", typeName, change.spriteIndex);
                outputBuffer += tempBuffer;
            }
            else if (change.changeType == GameStateSpriteChange_t::MODIFIED)
            {
                snprintf(
                    tempBuffer, sizeof(tempBuffer), "Sprite modifications (%s), index: %u\n", typeName, change.spriteIndex);
                outputBuffer += tempBuffer;
                for (auto& diff : change.diffs)
                {
                    snprintf(
                        tempBuffer, sizeof(tempBuffer),
                        "  %s::%s, len = %u, offset = %u, left = 0x%.16llX, right = 0x%.16llX\n", diff.structname,
                        diff.fieldname, (uint32_t)diff.length, (uint32_t)diff.offset, (unsigned long long)diff.valueA,
                        (unsigned long long)diff.valueB);
                    outputBuffer += tempBuffer;
                }
            }
        }

        FILE* fp = fopen(fileName.c_str(), "wt");
        if (!fp)
            return false;

        fputs(outputBuffer.c_str(), fp);
        fclose(fp);

        return true;
    }

private:
    CircularBuffer<std::unique_ptr<GameStateSnapshot_t>, MaximumGameStateSnapshots> _snapshots;
};

std::unique_ptr<IGameStateSnapshots> CreateGameStateSnapshots()
{
    return std::make_unique<GameStateSnapshots>();
}
