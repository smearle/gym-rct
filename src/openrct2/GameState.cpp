/*****************************************************************************
 * Copyright (c) 2014-2019 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "GameState.h"

#include "Context.h"
#include "Editor.h"
#include "Game.h"
#include "GameStateSnapshots.h"
#include "Input.h"
#include "OpenRCT2.h"
#include "ReplayManager.h"
#include "actions/GameAction.h"
#include "config/Config.h"
#include "interface/Screenshot.h"
#include "localisation/Date.h"
#include "localisation/Localisation.h"
#include "management/NewsItem.h"
#include "network/network.h"
#include "peep/Staff.h"
#include "platform/Platform2.h"
#include "scenario/Scenario.h"
#include "title/TitleScreen.h"
#include "title/TitleSequencePlayer.h"
#include "ui/UiContext.h"
#include "windows/Intent.h"
#include "world/Climate.h"
#include "world/MapAnimation.h"
#include "world/Park.h"
#include "world/Scenery.h"

#include <algorithm>

//#include "windows/Intent.h"
// AGENT
#include "Cheats.h"
#include "actions/RideCreateAction.hpp"
#include "actions/FootpathPlaceAction.hpp"
#include "actions/PlaceParkEntranceAction.hpp"
#include "actions/LandSetHeightAction.hpp"
#include "actions/MazeSetTrackAction.hpp"
#include "actions/WaterSetHeightAction.hpp"
#include "actions/TrackPlaceAction.hpp"
#include "actions/TrackDesignAction.h"
#include "ride/RideData.h"
#include "ride/RideGroupManager.h"
#include "ride/ShopItem.h"
#include "ride/Station.h"
#include "ride/Track.h"
#include "ride/TrackData.h"
#include "ride/TrackDesign.h"
#include "interface/Window.h"

using namespace OpenRCT2;
using namespace OpenRCT2::Ui;

GameState::GameState()
{
    _park = std::make_unique<Park>();
}

/**
 * Initialises the map, park etc. basically all S6 data.
 */
void GameState::InitAll(int32_t mapSize)
{
    gInMapInitCode = true;

    map_init(mapSize);
    _park->Initialise();
    finance_init();
    banner_init();
    ride_init_all();
    reset_sprite_list();
    staff_reset_modes();
    date_reset();
    climate_reset(CLIMATE_COOL_AND_WET);
    news_item_init_queue();

    gInMapInitCode = false;

    gNextGuestNumber = 1;

    context_init();
    scenery_set_default_placement_configuration();

    auto intent = Intent(INTENT_ACTION_CLEAR_TILE_INSPECTOR_CLIPBOARD);
    context_broadcast_intent(&intent);

    load_palette();

    // FIXME: Why doesn't this work?
    gParkFlags |= PARK_FLAGS_NO_MONEY;
  //auto setCheatAction = SetCheatAction();
  //auto result3 = GameActions::Execute(&setCheatAction);
}

/**
 * Function will be called every GAME_UPDATE_TIME_MS.
 * It has its own loop which might run multiple updates per call such as
 * when operating as a client it may run multiple updates to catch up with the server tick,
 * another influence can be the game speed setting.
 */
RideManager ride_manager = GetRideManager();
int num_actions = 3;
int act_i = 0;
uint8_t track_i = 76;
void GameState::Update()
{

    // AGENT TESTING
    // FIXME: remove this cash-reload
    gCash = 1000000;
    int x_tiles = gMapSizeUnits;
    int y_tiles = gMapSizeUnits;
    int ax = rand() % x_tiles;
    int ay = rand() % y_tiles;
    // The default height
  //int az = 115;
  //GetContext()->WriteLine(std::to_string(ax));
  //GetContext()->WriteLine(std::to_string(ay));
  //GetContext()->WriteLine(std::to_string(az));
  //int32_t selectedType;
  //selectedType = (gFootpathSelectedType << 7) + (gFootpathSelectedId & 0xFF);

    int az = rand() % MAXIMUM_LAND_HEIGHT + MINIMUM_LAND_HEIGHT;
  //if (act_i == 0) {
  //    auto landSetHeightAction = LandSetHeightAction({ax, ay}, az, 0);
  //  //auto result_height = GameActions::Execute(&landSetHeightAction);
  //}
  //if (act_i == 1) {
  //    int az = rand() % MAXIMUM_WATER_HEIGHT + MINIMUM_WATER_HEIGHT;
  //    auto waterHeightAction = WaterSetHeightAction({ax, ay}, az);
  //    auto result_water = GameActions::Execute(&waterHeightAction);
  //            }
  //if (act_i == 0) {
  //    int az = rand() % MAXIMUM_LAND_HEIGHT + MINIMUM_LAND_HEIGHT;
  //    auto footpathPlaceAction = FootpathPlaceAction({ax,ay,az}, 0, 0);
  //    auto result_footpath = GameActions::Execute(&footpathPlaceAction);
  //            }
  //if (act_i == 1) {
  //  //uint8_t ride_type_i;
  //  //uint8_t entry_i;
  //   
  //    int screen_x = rand() % 640;
  //  //screen_x = screen_x - 1;
  //    int screen_y = rand() % 360;
  //  //screen_y = screen_y - 1;
  //  //ScreenCoordsXY cursor_pos = {screen_x, screen_y};
  //  //context_set_cursor_position(cursor_pos);
  //    ride_list_item *listItem = new ride_list_item();
  //    listItem->type = 27;
  //    listItem->entry_index = 3;
  //    ride_construct_new(*listItem);
  //  //gScreenFlags = 68;
  //  //auto ride = get_ride(_currentRideIndex);
  //  //rct_string_id td = TrackDesign::CreateTrackDesign(ride);
  //  //auto trackPlaceAction = TrackPlaceAction();
////}
////if (act_i == 2) {
  //    auto trackPlaceAction = TrackPlaceAction(_currentRideIndex++, track_i, {ax, ay, az, 0}, 0, 0, 4, 0, false);
  //    auto result_track = GameActions::Execute(&trackPlaceAction);
  //}
      //for (track_i = 0; track_i < 200; track_i ++) {

      //auto trackPlaceAction2 = TrackPlaceAction(_currentRideIndex, track_i, {ax, ay, 136, 0}, 0, 0, 4, 0, false);
      //auto result_track2 = GameActions::Execute(&trackPlaceAction2);
      //}
        window_close_construction_windows();
      //track_i += 1;
        
        //FIXME: Why is this function undeclared?
        //Window::window_ride_construction_keyboard_shortcut_build_current();
            
        
      //rct_string_id td = TrackDesign::CreateTrackDesign(ride);
      //auto trackDesignAction = TrackDesignAction({ax, ay, az, 0}, td);
      //auto result_track = GameActions::Execute(&trackDesignAction);
      //int az = rand() % MAXIMUM_LAND_HEIGHT + MINIMUM_LAND_HEIGHT;
      //int32_t* trackType = 0;
      //int32_t* trackDirection = 0;
      //ride_id_t* rideIndex = &_currentRideIndex;
      //int32_t* _liftHillAndAlternativeState = 0;
      //int32_t* properties = 0;
      //window_ride_construction_update_state(
      //trackType, trackDirection, rideIndex, _liftHillAndAlternativeState, &ax,
      //&ay, &az, properties);

      //int32_t ride_type = 1;
      //auto rideCreateAction = RideCreateAction(ride_type, 3, 0, 0);
      //auto result_ride = GameActions::Execute(&rideCreateAction);
      //int az = rand() % MAXIMUM_LAND_HEIGHT + MINIMUM_LAND_HEIGHT;
    
  //act_i = (act_i + 1);
    act_i = (act_i + 1) % num_actions;
  //window_maze_construction_open();
    gInUpdateCode = true;
  //GetContext()->WriteLine(std::to_string(*result));
    // END AGENT TESTING //


    // Normal game play will update only once every GAME_UPDATE_TIME_MS
    uint32_t numUpdates = 1;

    // 0x006E3AEC // screen_game_process_mouse_input();
    screenshot_check();
    game_handle_keyboard_input();

    if (game_is_not_paused() && gPreviewingTitleSequenceInGame)
    {
        auto player = GetContext()->GetUiContext()->GetTitleSequencePlayer();
        if (player != nullptr)
        {
            player->Update();
        }
    }

    uint32_t realtimeTicksElapsed = gCurrentDeltaTime / GAME_UPDATE_TIME_MS;
    realtimeTicksElapsed = std::clamp<uint32_t>(realtimeTicksElapsed, 1, GAME_MAX_UPDATES);

    // We use this variable to always advance ticks in normal speed.
    gCurrentRealTimeTicks += realtimeTicksElapsed;

    network_update();

    if (network_get_mode() == NETWORK_MODE_CLIENT && network_get_status() == NETWORK_STATUS_CONNECTED
        && network_get_authstatus() == NETWORK_AUTH_OK)
    {
        numUpdates = std::clamp<uint32_t>(network_get_server_tick() - gCurrentTicks, 0, 10);
    }
    else
    {
        // Determine how many times we need to update the game
        if (gGameSpeed > 1)
        {
            // Update more often if game speed is above normal.
            numUpdates = 1 << (gGameSpeed - 1);
        }
    }

    bool isPaused = game_is_paused();
    if (network_get_mode() == NETWORK_MODE_SERVER && gConfigNetwork.pause_server_if_no_clients)
    {
        // If we are headless we always have 1 player (host), pause if no one else is around.
        if (gOpenRCT2Headless && network_get_num_players() == 1)
        {
            isPaused |= true;
        }
    }

    bool didRunSingleFrame = false;
    if (isPaused)
    {
        if (gDoSingleUpdate && network_get_mode() == NETWORK_MODE_NONE)
        {
            didRunSingleFrame = true;
            pause_toggle();
            numUpdates = 1;
        }
        else
        {
            numUpdates = 0;
            // Update the animation list. Note this does not
            // increment the map animation.
            map_animation_invalidate_all();

            // Special case because we set numUpdates to 0, otherwise in game_logic_update.
            network_process_pending();

            GameActions::ProcessQueue();
        }
    }

    // Update the game one or more times
    for (uint32_t i = 0; i < numUpdates; i++)
    {
        UpdateLogic();
        if (gGameSpeed == 1)
        {
            if (input_get_state() == INPUT_STATE_RESET || input_get_state() == INPUT_STATE_NORMAL)
            {
                if (input_test_flag(INPUT_FLAG_VIEWPORT_SCROLLING))
                {
                    input_set_flag(INPUT_FLAG_VIEWPORT_SCROLLING, false);
                    break;
                }
            }
            else
            {
                break;
            }
        }
    }

    if (!gOpenRCT2Headless)
    {
        input_set_flag(INPUT_FLAG_VIEWPORT_SCROLLING, false);

        // the flickering frequency is reduced by 4, compared to the original
        // it was done due to inability to reproduce original frequency
        // and decision that the original one looks too fast
        if (gCurrentRealTimeTicks % 4 == 0)
            gWindowMapFlashingFlags ^= (1 << 15);

        // Handle guest map flashing
        gWindowMapFlashingFlags &= ~(1 << 1);
        if (gWindowMapFlashingFlags & (1 << 0))
            gWindowMapFlashingFlags |= (1 << 1);
        gWindowMapFlashingFlags &= ~(1 << 0);

        // Handle staff map flashing
        gWindowMapFlashingFlags &= ~(1 << 3);
        if (gWindowMapFlashingFlags & (1 << 2))
            gWindowMapFlashingFlags |= (1 << 3);
        gWindowMapFlashingFlags &= ~(1 << 2);

        context_update_map_tooltip();

        context_handle_input();
    }

    // Always perform autosave check, even when paused
    if (!(gScreenFlags & SCREEN_FLAGS_TITLE_DEMO) && !(gScreenFlags & SCREEN_FLAGS_TRACK_DESIGNER)
        && !(gScreenFlags & SCREEN_FLAGS_TRACK_MANAGER))
    {
        scenario_autosave_check();
    }

    window_dispatch_update_all();

    if (didRunSingleFrame && game_is_not_paused() && !(gScreenFlags & SCREEN_FLAGS_TITLE_DEMO))
    {
        pause_toggle();
    }

    gDoSingleUpdate = false;
    gInUpdateCode = false;
}

void GameState::UpdateLogic()
{
    gScreenAge++;
    if (gScreenAge == 0)
        gScreenAge--;

    GetContext()->GetReplayManager()->Update();

    network_update();

    if (network_get_mode() == NETWORK_MODE_SERVER)
    {
        if (network_gamestate_snapshots_enabled())
        {
            CreateStateSnapshot();
        }

        // Send current tick out.
        network_send_tick();
    }
    else if (network_get_mode() == NETWORK_MODE_CLIENT)
    {
        // Don't run past the server, this condition can happen during map changes.
        if (network_get_server_tick() == gCurrentTicks)
        {
            return;
        }

        // Check desync.
        bool desynced = network_check_desynchronisation();
        if (desynced)
        {
            // If desync debugging is enabled and we are still connected request the specific game state from server.
            if (network_gamestate_snapshots_enabled() && network_get_status() == NETWORK_STATUS_CONNECTED)
            {
                // Create snapshot from this tick so we can compare it later
                // as we won't pause the game on this event.
                CreateStateSnapshot();

                network_request_gamestate_snapshot();
            }
        }
    }

    date_update();
    _date = Date(gDateMonthTicks, gDateMonthTicks);

    scenario_update();
    climate_update();
    map_update_tiles();
    // Temporarily remove provisional paths to prevent peep from interacting with them
    map_remove_provisional_elements();
    map_update_path_wide_flags();
    peep_update_all();
    map_restore_provisional_elements();
    vehicle_update_all();
    sprite_misc_update_all();
    Ride::UpdateAll();

    if (!(gScreenFlags & SCREEN_FLAGS_EDITOR))
    {
        _park->Update(_date);
    }

    research_update();
    ride_ratings_update_all();
    ride_measurements_update();
    news_item_update_current();

    map_animation_invalidate_all();
    vehicle_sounds_update();
    peep_update_crowd_noise();
    climate_update_sound();
    editor_open_windows_for_current_step();

    // Update windows
    // window_dispatch_update_all();

    // Start autosave timer after update
    if (gLastAutoSaveUpdate == AUTOSAVE_PAUSE)
    {
        gLastAutoSaveUpdate = Platform::GetTicks();
    }

    GameActions::ProcessQueue();

    network_process_pending();
    network_flush();

    gCurrentTicks++;
    gScenarioTicks++;
    gSavedAge++;
}

void GameState::CreateStateSnapshot()
{
    IGameStateSnapshots* snapshots = GetContext()->GetGameStateSnapshots();

    auto& snapshot = snapshots->CreateSnapshot();
    snapshots->Capture(snapshot);
    snapshots->LinkSnapshot(snapshot, gCurrentTicks, scenario_rand_state().s0);
}
