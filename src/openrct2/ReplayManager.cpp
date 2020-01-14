/*****************************************************************************
 * Copyright (c) 2014-2019 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "ReplayManager.h"

#include "Context.h"
#include "Game.h"
#include "OpenRCT2.h"
#include "ParkImporter.h"
#include "PlatformEnvironment.h"
#include "actions/FootpathPlaceAction.hpp"
#include "actions/GameAction.h"
#include "actions/RideEntranceExitPlaceAction.hpp"
#include "actions/RideSetSetting.hpp"
#include "actions/SetCheatAction.hpp"
#include "actions/TileModifyAction.hpp"
#include "actions/TrackPlaceAction.hpp"
#include "config/Config.h"
#include "core/DataSerialiser.h"
#include "core/Path.hpp"
#include "management/NewsItem.h"
#include "object/ObjectManager.h"
#include "object/ObjectRepository.h"
#include "rct2/S6Exporter.h"
#include "world/Park.h"
#include "zlib.h"

#include <chrono>
#include <memory>
#include <vector>

namespace OpenRCT2
{
    struct ReplayCommand
    {
        uint32_t tick = 0;
        GameAction::Ptr action;
        uint32_t commandIndex = 0;

        ReplayCommand() = default;

        ReplayCommand(uint32_t t, std::unique_ptr<GameAction>&& ga, uint32_t id)
            : tick(t)
            , action(std::move(ga))
            , commandIndex(id)

        {
        }

        bool operator<(const ReplayCommand& comp) const
        {
            // First sort by tick
            if (tick < comp.tick)
                return true;
            if (tick > comp.tick)
                return false;

            // If the ticks are equal sort by commandIndex
            return commandIndex < comp.commandIndex;
        }
    };

    struct ReplayRecordFile
    {
        uint32_t magic;
        uint16_t version;
        uint64_t uncompressedSize;
        MemoryStream data;
    };

    struct ReplayRecordData
    {
        uint32_t magic;
        uint16_t version;
        std::string networkId;
        MemoryStream parkData;
        MemoryStream spriteSpatialData;
        MemoryStream parkParams;
        MemoryStream cheatData;
        std::string name;      // Name of play
        std::string filePath;  // File path of replay.
        uint64_t timeRecorded; // Posix Time.
        uint32_t tickStart;    // First tick of replay.
        uint32_t tickEnd;      // Last tick of replay.
        std::multiset<ReplayCommand> commands;
        std::vector<std::pair<uint32_t, rct_sprite_checksum>> checksums;
        uint32_t checksumIndex;
    };

    class ReplayManager final : public IReplayManager
    {
        static constexpr uint16_t ReplayVersion = 3;
        static constexpr uint32_t ReplayMagic = 0x5243524F; // ORCR.
        static constexpr int ReplayCompressionLevel = 9;

        enum class ReplayMode
        {
            NONE = 0,
            RECORDING,
            PLAYING,
            NORMALISATION,
        };

    public:
        virtual ~ReplayManager()
        {
        }

        virtual bool IsReplaying() const override
        {
            return _mode == ReplayMode::PLAYING;
        }

        virtual bool IsRecording() const override
        {
            return _mode == ReplayMode::RECORDING;
        }

        virtual bool IsNormalising() const override
        {
            return _mode == ReplayMode::NORMALISATION;
        }

        virtual void AddGameAction(uint32_t tick, const GameAction* action) override
        {
            if (_currentRecording == nullptr)
                return;

            auto ga = GameActions::Clone(action);

            _currentRecording->commands.emplace(gCurrentTicks, std::move(ga), _commandId++);
        }

        void AddChecksum(uint32_t tick, rct_sprite_checksum&& checksum)
        {
            _currentRecording->checksums.emplace_back(std::make_pair(tick, checksum));
        }

        // Function runs each Tick.
        virtual void Update() override
        {
            if (_mode == ReplayMode::NONE)
                return;

            if ((_mode == ReplayMode::RECORDING || _mode == ReplayMode::NORMALISATION) && gCurrentTicks == _nextChecksumTick)
            {
                rct_sprite_checksum checksum = sprite_checksum();
                AddChecksum(gCurrentTicks, std::move(checksum));

                _nextChecksumTick = gCurrentTicks + 1;
            }

            if (_mode == ReplayMode::RECORDING)
            {
                if (gCurrentTicks >= _currentRecording->tickEnd)
                {
                    StopRecording();
                    return;
                }
            }
            else if (_mode == ReplayMode::PLAYING)
            {
#ifndef DISABLE_NETWORK
                // If the network is disabled we will only get a dummy hash which will cause
                // false positives during replay.
                CheckState();
#endif
                ReplayCommands();

                // Normal playback will always end at the specific tick.
                if (gCurrentTicks >= _currentReplay->tickEnd)
                {
                    StopPlayback();
                    return;
                }
            }
            else if (_mode == ReplayMode::NORMALISATION)
            {
                ReplayCommands();

                // If we run out of commands we can just stop
                if (_currentReplay->commands.empty())
                {
                    StopPlayback();
                    StopRecording();

                    // Reset mode, in normalisation nothing will set it.
                    _mode = ReplayMode::NONE;
                    return;
                }
            }
        }

        virtual bool StartRecording(const std::string& name, uint32_t maxTicks /*= k_MaxReplayTicks*/) override
        {
            if (_mode != ReplayMode::NONE && _mode != ReplayMode::NORMALISATION)
                return false;

            auto replayData = std::make_unique<ReplayRecordData>();
            replayData->magic = ReplayMagic;
            replayData->version = ReplayVersion;
            replayData->networkId = network_get_version();
            replayData->name = name;
            replayData->tickStart = gCurrentTicks;
            if (maxTicks != k_MaxReplayTicks)
                replayData->tickEnd = gCurrentTicks + maxTicks;
            else
                replayData->tickEnd = k_MaxReplayTicks;

            std::string replayName = String::StdFormat("%s.sv6r", name.c_str());
            std::string outPath = GetContext()->GetPlatformEnvironment()->GetDirectoryPath(DIRBASE::USER, DIRID::REPLAY);
            replayData->filePath = Path::Combine(outPath, replayName);

            auto context = GetContext();
            auto& objManager = context->GetObjectManager();
            auto objects = objManager.GetPackableObjects();

            auto s6exporter = std::make_unique<S6Exporter>();
            s6exporter->ExportObjectsList = objects;
            s6exporter->Export();
            s6exporter->SaveGame(&replayData->parkData);

            replayData->spriteSpatialData.Write(gSpriteSpatialIndex, sizeof(gSpriteSpatialIndex));
            replayData->timeRecorded = std::chrono::seconds(std::time(nullptr)).count();

            DataSerialiser parkParamsDs(true, replayData->parkParams);
            SerialiseParkParameters(parkParamsDs);

            DataSerialiser cheatDataDs(true, replayData->cheatData);
            SerialiseCheats(cheatDataDs);

            if (_mode != ReplayMode::NORMALISATION)
                _mode = ReplayMode::RECORDING;

            _currentRecording = std::move(replayData);
            _nextChecksumTick = gCurrentTicks + 1;

            return true;
        }

        virtual bool StopRecording() override
        {
            if (_mode != ReplayMode::RECORDING && _mode != ReplayMode::NORMALISATION)
                return false;

            _currentRecording->tickEnd = gCurrentTicks;

            // Serialise Body.
            DataSerialiser recSerialiser(true);
            Serialise(recSerialiser, *_currentRecording);

            const auto& stream = recSerialiser.GetStream();
            unsigned long streamLength = static_cast<unsigned long>(stream.GetLength());
            unsigned long compressLength = compressBound(streamLength);

            MemoryStream data(compressLength);

            ReplayRecordFile file{ _currentRecording->magic, _currentRecording->version, streamLength, data };

            auto compressBuf = std::make_unique<unsigned char[]>(compressLength);
            compress2(
                compressBuf.get(), &compressLength, (unsigned char*)stream.GetData(), stream.GetLength(),
                ReplayCompressionLevel);
            file.data.Write(compressBuf.get(), compressLength);

            DataSerialiser fileSerialiser(true);
            fileSerialiser << file.magic;
            fileSerialiser << file.version;
            fileSerialiser << file.uncompressedSize;
            fileSerialiser << file.data;

            bool result = false;

            const std::string& outFile = _currentRecording->filePath;

            FILE* fp = fopen(outFile.c_str(), "wb");
            if (fp)
            {
                const auto& fileStream = fileSerialiser.GetStream();
                fwrite(fileStream.GetData(), 1, fileStream.GetLength(), fp);
                fclose(fp);

                result = true;
            }
            else
            {
                log_error("Unable to write to file '%s'", outFile.c_str());
                result = false;
            }

            // When normalizing the output we don't touch the mode.
            if (_mode != ReplayMode::NORMALISATION)
                _mode = ReplayMode::NONE;

            _currentRecording.reset();

            NewsItem* news = news_item_add_to_queue_raw(NEWS_ITEM_BLANK, "Replay recording stopped", 0);
            news->Flags |= NEWS_FLAG_HAS_BUTTON; // Has no subject.

            return result;
        }

        virtual bool GetCurrentReplayInfo(ReplayRecordInfo& info) const override
        {
            ReplayRecordData* data = nullptr;

            if (_mode == ReplayMode::PLAYING)
                data = _currentReplay.get();
            else if (_mode == ReplayMode::RECORDING)
                data = _currentRecording.get();
            else if (_mode == ReplayMode::NORMALISATION)
                data = _currentRecording.get();

            if (data == nullptr)
                return false;

            info.FilePath = data->filePath;
            info.Name = data->name;
            info.Version = data->version;
            info.TimeRecorded = data->timeRecorded;
            if (_mode == ReplayMode::RECORDING)
                info.Ticks = gCurrentTicks - data->tickStart;
            else if (_mode == ReplayMode::PLAYING)
                info.Ticks = data->tickEnd - data->tickStart;
            info.NumCommands = (uint32_t)data->commands.size();
            info.NumChecksums = (uint32_t)data->checksums.size();

            return true;
        }

        virtual bool StartPlayback(const std::string& file) override
        {
            if (_mode != ReplayMode::NONE && _mode != ReplayMode::NORMALISATION)
                return false;

            auto replayData = std::make_unique<ReplayRecordData>();

            if (!ReadReplayData(file, *replayData))
            {
                log_error("Unable to read replay data.");
                return false;
            }

            if (!LoadReplayDataMap(*replayData))
            {
                log_error("Unable to load map.");
                return false;
            }

            gCurrentTicks = replayData->tickStart;

            _currentReplay = std::move(replayData);
            _currentReplay->checksumIndex = 0;
            _faultyChecksumIndex = -1;

            // Make sure game is not paused.
            gGamePaused = 0;

            if (_mode != ReplayMode::NORMALISATION)
                _mode = ReplayMode::PLAYING;

            return true;
        }

        virtual bool IsPlaybackStateMismatching() const override
        {
            if (_mode != ReplayMode::PLAYING)
            {
                return false;
            }
            return _faultyChecksumIndex != -1;
        }

        virtual bool StopPlayback() override
        {
            if (_mode != ReplayMode::PLAYING && _mode != ReplayMode::NORMALISATION)
                return false;

            // During normal playback we pause the game if stopped.
            if (_mode == ReplayMode::PLAYING)
            {
                NewsItem* news = news_item_add_to_queue_raw(NEWS_ITEM_BLANK, "Replay playback complete", 0);
                news->Flags |= NEWS_FLAG_HAS_BUTTON; // Has no subject.
            }

            // When normalizing the output we don't touch the mode.
            if (_mode != ReplayMode::NORMALISATION)
            {
                _mode = ReplayMode::NONE;
            }

            _currentReplay.reset();

            return true;
        }

        virtual bool NormaliseReplay(const std::string& file, const std::string& outFile) override
        {
            _mode = ReplayMode::NORMALISATION;

            if (!StartPlayback(file))
            {
                return false;
            }

            if (!StartRecording(outFile, k_MaxReplayTicks))
            {
                StopPlayback();
                return false;
            }

            _nextReplayTick = gCurrentTicks + 1;

            return true;
        }

    private:
        bool LoadReplayDataMap(ReplayRecordData& data)
        {
            try
            {
                data.parkData.SetPosition(0);

                auto context = GetContext();
                auto& objManager = context->GetObjectManager();
                auto importer = ParkImporter::CreateS6(context->GetObjectRepository());

                auto loadResult = importer->LoadFromStream(&data.parkData, false);
                objManager.LoadObjects(loadResult.RequiredObjects.data(), loadResult.RequiredObjects.size());

                importer->Import();

                sprite_position_tween_reset();

                Guard::Assert(sizeof(gSpriteSpatialIndex) >= data.spriteSpatialData.GetLength());

                // In case the sprite limit will be increased we keep the unused fields cleared.
                std::fill_n(gSpriteSpatialIndex, std::size(gSpriteSpatialIndex), SPRITE_INDEX_NULL);
                std::memcpy(gSpriteSpatialIndex, data.spriteSpatialData.GetData(), data.spriteSpatialData.GetLength());

                // Load all map global variables.
                DataSerialiser parkParamsDs(false, data.parkParams);
                SerialiseParkParameters(parkParamsDs);

                // New cheats might not be serialised, make sure they are using their defaults.
                CheatsReset();

                DataSerialiser cheatDataDs(false, data.cheatData);
                SerialiseCheats(cheatDataDs);

                game_load_init();
                fix_invalid_vehicle_sprite_sizes();
            }
            catch (const std::exception& ex)
            {
                log_error("Exception: %s", ex.what());
                return false;
            }
            return true;
        }

        bool ReadReplayFromFile(const std::string& file, MemoryStream& stream)
        {
            FILE* fp = fopen(file.c_str(), "rb");
            if (!fp)
                return false;

            char buffer[128];
            while (feof(fp) == 0)
            {
                size_t numBytesRead = fread(buffer, 1, 128, fp);
                if (numBytesRead == 0)
                    break;
                stream.Write(buffer, numBytesRead);
            }

            fclose(fp);
            return true;
        }

        /**
         * Returns true if decompression was not needed or succeeded
         * @param stream
         * @return
         */
        bool TryDecompress(MemoryStream& stream)
        {
            ReplayRecordFile recFile;
            stream.SetPosition(0);
            DataSerialiser fileSerializer(false, stream);
            fileSerializer << recFile.magic;
            fileSerializer << recFile.version;

            if (recFile.version >= 2)
            {
                fileSerializer << recFile.uncompressedSize;
                fileSerializer << recFile.data;

                auto buff = std::make_unique<unsigned char[]>(recFile.uncompressedSize);
                unsigned long outSize = recFile.uncompressedSize;
                uncompress(
                    (unsigned char*)buff.get(), &outSize, (unsigned char*)recFile.data.GetData(), recFile.data.GetLength());
                if (outSize != recFile.uncompressedSize)
                {
                    return false;
                }
                stream.SetPosition(0);
                stream.Write(buff.get(), outSize);
            }

            return true;
        }

        bool ReadReplayData(const std::string& file, ReplayRecordData& data)
        {
            MemoryStream stream;

            std::string fileName = file;
            if (fileName.size() < 5 || fileName.substr(fileName.size() - 5) != ".sv6r")
            {
                fileName += ".sv6r";
            }

            std::string outPath = GetContext()->GetPlatformEnvironment()->GetDirectoryPath(DIRBASE::USER, DIRID::REPLAY);
            std::string outFile = Path::Combine(outPath, fileName);

            bool loaded = false;
            if (ReadReplayFromFile(outFile, stream))
            {
                data.filePath = outFile;
                loaded = true;
            }
            else if (ReadReplayFromFile(file, stream))
            {
                data.filePath = file;
                loaded = true;
            }
            if (!loaded)
                return false;

            if (!TryDecompress(stream))
                return false;

            stream.SetPosition(0);
            DataSerialiser serialiser(false, stream);
            if (!Serialise(serialiser, data))
            {
                return false;
            }

            // Reset position of all streams.
            data.parkData.SetPosition(0);
            data.parkParams.SetPosition(0);
            data.cheatData.SetPosition(0);
            data.spriteSpatialData.SetPosition(0);

            return true;
        }

        bool SerialiseCheats(DataSerialiser& serialiser)
        {
            CheatsSerialise(serialiser);

            return true;
        }

        bool SerialiseParkParameters(DataSerialiser& serialiser)
        {
            serialiser << _guestGenerationProbability;
            serialiser << _suggestedGuestMaximum;
            serialiser << gConfigGeneral.show_real_names_of_guests;

            // To make this a little bit less volatile against updates
            // we reserve some space for future additions.
            uint64_t tempStorage = 0;

            // If another park parameter has to be added simply swap tempStorage.
            // and ensure the length read/write will stay uint64_t
            serialiser << tempStorage;
            serialiser << tempStorage;
            serialiser << tempStorage;
            serialiser << tempStorage;
            serialiser << tempStorage;
            serialiser << tempStorage;
            serialiser << tempStorage;
            serialiser << tempStorage;

            return true;
        }

        bool SerialiseCommand(DataSerialiser& serialiser, ReplayCommand& command)
        {
            serialiser << command.tick;
            serialiser << command.commandIndex;

            uint32_t actionType = 0;
            if (serialiser.IsSaving())
            {
                actionType = command.action->GetType();
            }
            serialiser << actionType;

            if (serialiser.IsLoading())
            {
                command.action = GameActions::Create(actionType);
                Guard::Assert(command.action != nullptr);
            }

            command.action->Serialise(serialiser);

            return true;
        }

        bool Compatible(ReplayRecordData& data)
        {
            return data.version == ReplayVersion;
        }

        bool Serialise(DataSerialiser& serialiser, ReplayRecordData& data)
        {
            serialiser << data.magic;
            if (data.magic != ReplayMagic)
            {
                log_error("Magic does not match %08X, expected: %08X", data.magic, ReplayMagic);
                return false;
            }
            serialiser << data.version;
            if (data.version != ReplayVersion && !Compatible(data))
            {
                log_error("Invalid version detected %04X, expected: %04X", data.version, ReplayVersion);
                return false;
            }

            serialiser << data.networkId;
#ifndef DISABLE_NETWORK
            // NOTE: This does not mean the replay will not function, only a warning.
            if (data.networkId != network_get_version())
            {
                log_warning(
                    "Replay network version mismatch: '%s', expected: '%s'", data.networkId.c_str(),
                    network_get_version().c_str());
            }
#endif

            serialiser << data.name;
            serialiser << data.timeRecorded;
            serialiser << data.parkData;
            serialiser << data.parkParams;
            serialiser << data.cheatData;
            serialiser << data.spriteSpatialData;
            serialiser << data.tickStart;
            serialiser << data.tickEnd;

            uint32_t countCommands = (uint32_t)data.commands.size();
            serialiser << countCommands;

            if (serialiser.IsSaving())
            {
                for (auto& command : data.commands)
                {
                    SerialiseCommand(serialiser, const_cast<ReplayCommand&>(command));
                }
            }
            else
            {
                for (uint32_t i = 0; i < countCommands; i++)
                {
                    ReplayCommand command = {};
                    SerialiseCommand(serialiser, command);

                    data.commands.emplace(std::move(command));
                }
            }

            uint32_t countChecksums = (uint32_t)data.checksums.size();
            serialiser << countChecksums;

            if (serialiser.IsLoading())
            {
                data.checksums.resize(countChecksums);
            }

            for (uint32_t i = 0; i < countChecksums; i++)
            {
                serialiser << data.checksums[i].first;
                serialiser << data.checksums[i].second.raw;
            }

            return true;
        }

#ifndef DISABLE_NETWORK
        void CheckState()
        {
            if (_nextChecksumTick != gCurrentTicks)
                return;

            uint32_t checksumIndex = _currentReplay->checksumIndex;

            if (checksumIndex >= _currentReplay->checksums.size())
                return;

            const auto& savedChecksum = _currentReplay->checksums[checksumIndex];
            if (_currentReplay->checksums[checksumIndex].first == gCurrentTicks)
            {
                rct_sprite_checksum checksum = sprite_checksum();
                if (savedChecksum.second.raw != checksum.raw)
                {
                    uint32_t replayTick = gCurrentTicks - _currentReplay->tickStart;

                    // Detected different game state.
                    log_warning(
                        "Different sprite checksum at tick %u (Replay Tick: %u) ; Saved: %s, Current: %s", gCurrentTicks,
                        replayTick, savedChecksum.second.ToString().c_str(), checksum.ToString().c_str());

                    _faultyChecksumIndex = checksumIndex;
                }
                else
                {
                    // Good state.
                    log_verbose(
                        "Good state at tick %u ; Saved: %s, Current: %s", gCurrentTicks,
                        savedChecksum.second.ToString().c_str(), checksum.ToString().c_str());
                }
                _currentReplay->checksumIndex++;
            }
        }
#endif // DISABLE_NETWORK

        void ReplayCommands()
        {
            auto& replayQueue = _currentReplay->commands;

            while (replayQueue.begin() != replayQueue.end())
            {
                const ReplayCommand& command = (*replayQueue.begin());

                if (_mode == ReplayMode::PLAYING)
                {
                    // If this is a normal playback wait for the correct tick.
                    if (command.tick != gCurrentTicks)
                        break;
                }
                else if (_mode == ReplayMode::NORMALISATION)
                {
                    // Allow one entry per tick.
                    if (gCurrentTicks != _nextReplayTick)
                        break;

                    _nextReplayTick = gCurrentTicks + 1;
                }

                bool isPositionValid = false;

                GameAction* action = command.action.get();
                action->SetFlags(action->GetFlags() | GAME_COMMAND_FLAG_REPLAY);

                GameActionResult::Ptr result = GameActions::Execute(action);
                if (result->Error == GA_ERROR::OK)
                {
                    isPositionValid = true;
                }

                // Focus camera on event.
                if (isPositionValid && !result->Position.isNull())
                {
                    auto* mainWindow = window_get_main();
                    if (mainWindow != nullptr)
                        window_scroll_to_location(mainWindow, result->Position.x, result->Position.y, result->Position.z);
                }

                replayQueue.erase(replayQueue.begin());
            }
        }

    private:
        ReplayMode _mode = ReplayMode::NONE;
        std::unique_ptr<ReplayRecordData> _currentRecording;
        std::unique_ptr<ReplayRecordData> _currentReplay;
        int32_t _faultyChecksumIndex = -1;
        uint32_t _commandId = 0;
        uint32_t _nextChecksumTick = 0;
        uint32_t _nextReplayTick = 0;
    };

    std::unique_ptr<IReplayManager> CreateReplayManager()
    {
        return std::make_unique<ReplayManager>();
    }

} // namespace OpenRCT2
