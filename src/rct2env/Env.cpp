#include "UiContext.h"
#include <openrct2-ui/audio/AudioContext.h>
#include "../openrct2-ui/drawing/BitmapReader.h"
#include <openrct2/interface/Screenshot.h>
#include <openrct2/drawing/Drawing.h>
#include <openrct2/drawing/IDrawingEngine.h>
#include <openrct2/Context.h>
#include <openrct2/OpenRCT2.h>
#include <openrct2/PlatformEnvironment.h>
#include <openrct2/audio/AudioContext.h>
#include <openrct2/cmdline/CommandLine.hpp>
#include <openrct2/platform/platform.h>
#include <openrct2/ui/UiContext.h>
#include <openrct2/Cheats.h>
#include <spdlog/spdlog.h>
#include "Env.h"
#include "openrct2/drawing/NewDrawing.h"
#include <cxxabi.h>
#include <openrct2/interface/Viewport.h>
#include <openrct2/interface/Screenshot.h>
#include <openrct2/world/Map.h>
#include <openrct2/ride/Track.h>

#include <openrct2/actions/GameAction.h>
#include <openrct2/actions/RideCreateAction.hpp>
#include <openrct2/actions/TrackPlaceAction.hpp>
#include <openrct2/actions/RideSetAppearanceAction.hpp>
#include <openrct2/actions/RideSetStatus.hpp>
#include <openrct2/actions/RideEntranceExitPlaceAction.hpp>
#include <openrct2/actions/RideDemolishAction.hpp>
#include <openrct2/world/Location.hpp>
#include <torch/torch.h>
#include <algorithm>
#include <functional>
#include <math.h>

//AGENT
#include <rct2env/Agent.h>
#include <openrct2/ride/Ride.h>
#include <openrct2/interface/Window.h>
#include <openrct2/interface/Cursors.h>
//END AGENT

using namespace std;
using namespace OpenRCT2;
using namespace OpenRCT2::Audio;
using namespace OpenRCT2::Ui;
//using namespace cpprl;
//using namespace rctai;

template <typename T>
std::vector<T> operator+(const std::vector<T>& a, const std::vector<T>& b)
{
    assert(a.size() == b.size());

    std::vector<T> result;
    result.reserve(a.size());

    std::transform(a.begin(), a.end(), b.begin(), 
                   std::back_inserter(result), std::plus<T>());
    return result;
}


RCT2Env::RCT2Env()
{
}


EnvInfo * RCT2Env::GetInfo()
{
    this->agent->Configure(15, 15);
    n_chan = agent->len_by_type[TRACK_TYPE] + agent->len_by_type[MAP_Z];
    for (int i = 0; i < NUM_ACTION_TYPES; i++) {
        int t = (*this->agent).len_by_type.at(i);
//      std::cout << t << ' ';
    } 
    EnvInfo * env_info = new EnvInfo();
    env_info->observation_space_type = "Box";
    env_info->action_space_type = "MultiBinary";
    std::vector<long int> observation_space_shape = {n_chan, map_width, map_width};
  //Image image = get_observation();
  //std::vector<uint8_t> pixels = image.Pixels;
  //std::vector<long int> observation_space_shape = {pixels.size()};
  //std::vector<long int> observation_space_shape = {100};
    env_info->observation_space_shape = observation_space_shape;
    std::vector<long int>  action_space_shape = {agent->n_action_bins};
    printf("n action bins: %d", agent->n_action_bins);
    env_info->action_space_shape = action_space_shape;
    return env_info;
}

void RCT2Env::Init(int argc, const char** argv, Agent* agent)
{
  //std::unique_ptr<IContext> context;
    this->agent = agent;
    int32_t rc = EXIT_SUCCESS;
    int runGame = cmdline_run(argv, argc);
    core_init();
    RegisterBitmapReader();
	gCheatsIgnoreResearchStatus = true;
	gCheatsSandboxMode = true;
  //gOpenRCT2Headless = false; // run with GUI?
    if (runGame == EXITCODE_CONTINUE)
    {
  		spdlog::info("Creating context:");
        if (gOpenRCT2Headless)
        {
			spdlog::info("plain context");
            // Run OpenRCT2 with a plain context
            context = CreateContext();
        }
        else
        {
			//spdlog::info("ui context");
            // Run OpenRCT2 with a UI context
            auto env = to_shared(CreatePlatformEnvironment());
            auto audioContext = to_shared(CreateAudioContext());
            auto uiContext = to_shared(CreateUiContext(env));
            context = CreateContext(env, audioContext, uiContext);
        }
      rc = context->RunOpenRCT2(argc, argv);


    // Agent
    map_width = 15;
    int mouse_i;
    state = torch::zeros({n_chan, map_width, map_width}).to(torch::kBool);
    }
    else if (runGame == EXITCODE_FAIL)
    {
        rc = EXIT_FAILURE;
    }
}
torch::Tensor RCT2Env::Reset() {
    one_build = false;
    auto rideDemolishAction = RideDemolishAction(_currentRideIndex, 0);
    auto rideDemolishResult = GameActions::Execute(&rideDemolishAction);
    CoordsXYE build_trg;
    count = 0;
    state = torch::zeros({n_chan, map_width, map_width}).to(torch::kBool);
    this->n_step = 0;
    return Observe();
}

torch::Tensor RCT2Env::Observe() {
  //Image image = get_observation();
  //std::vector<uint8_t> pixels = image.Pixels;
  //torch::Tensor observation = torch::from_blob(pixels.data(), {1}).to(torch::kInt8);
  //for (int i = 0; i < pixels.size(); i++) {
  //    std::cout << unsigned(pixels.at(i)) << ' ';
  //}
  //std::cout << pixels.size();
  //return torch::zeros<this.observation_space_shape>;
  //std::vector<float> observation(100, 0.0);
  //std::vector<std::vector<float> > observation(
  //    200,
  //    std::vector<float>(300));
  //torch::Tensor observation = torch::randn({100});
    return state;
}

int get_action_from_bin(std::vector<std::vector<bool>> actions, Agent* agent, int ACT_TYPE, int* bin_j) {
    int n_bins = agent->len_by_type[ACT_TYPE];
  //printf("map_x_bins : %d\n", map_x_bins);
    int act_int = 0;
    for (int i = 0; i < n_bins; i++) {
        bool act_b = actions[0][i];
        act_int = act_int + act_b * pow(2, i);
        *bin_j += 1;
    }
    return act_int;

}

StepResult RCT2Env::Step(std::vector<std::vector<bool>> actions) {
    this->n_step += 1;
    bool actions_flat[actions[0].size()];
//  spdlog::info("stepping env");
    for (int i = 0; i < actions[0].size(); i++) {
     // std::cout << actions.at(0).at(i) << ' ';
     actions_flat[i] = actions[0][i];
    }
  //std::cout << std::endl;
    this->Observe();
  //uint8_t* bits = drawStepResult RCT2Env::Step(std::vector<std::vector<bool>> actions) {
  //uint8_t* bits = drawiing_engine_get_dpi()->bits;
  //spdlog::info( abi::__cxa_demangle(typeid(bits).name(), 0, 0, 0));
  //spdlog::info(&bits);
    this->context->RunFrame();
    rewards = torch::zeros({1, 1});
  //std::vector<bool> env_actions(agent.n_action_bins);
    int bins_i = 0;
    for (int i = 0; i < NUM_ACTION_TYPES; i++) {
        int t = (*this->agent).len_by_type.at(i);
      //std::cout << t << ' ';
    }
  //std::cout << std::endl;
  //bins_i += map_x_bins;
//    std::cout << map_x_bins << std::endl;
//  for (int i = 0; i < map_x_bins; i++) {
//      std::cout << actions.at(i) << ' ';
//  }
    int bin_j = 0;
    uint8_t map_x_grid = get_action_from_bin(actions, agent, MAP_X, &bin_j);
    uint8_t map_y_grid = get_action_from_bin(actions, agent, MAP_Y, &bin_j);
    int j_z = bin_j;
    int n_z_bins = agent->len_by_type[MAP_Z];
    int n_tt_bins = agent->len_by_type[TRACK_TYPE];
    uint8_t map_z_grid = get_action_from_bin(actions, agent, MAP_Z, &bin_j);
    uint8_t track_type = get_action_from_bin(actions, agent, TRACK_TYPE, &bin_j);
    uint8_t track_direction = get_action_from_bin(actions, agent, DIRECTION, &bin_j);

    // some kind of invisible track piece??
  //track_type = track_type % (TRACK_MINI_GOLF_HOLE - 1);
  //if ((50 <= track_type) && (track_type <= 53)) {
  //    printf("tt to constrain: %d\n", track_type);
  //    track_type -= 3;
  //    printf("tt after constrain: %d\n", track_type);
  //}
//  track_type = TRACK_NONE;
    int32_t map_x = map_x_grid;
    int32_t map_y = map_y_grid;
    map_x = map_x * COORDS_XY_STEP; 
    map_y = map_y * COORDS_XY_STEP; 
  //map_z = 7;
    int32_t map_z = map_z_grid * LAND_HEIGHT_STEP; 

    rideType = RIDE_TYPE_CORKSCREW_ROLLER_COASTER;
    rideSubType = 4;
  //map_x = rand() % gMapSizeUnits;
  //map_y = rand() % gMapSizeUnits;
  //map_z = rand() % gMapSizeUnits;
  //direction = rand() % 4;
  //map_y = rand() % gMapSizeUnits;
  //map_z = rand() % gMapSizeUnits;

  //track_type = rand() % 256;
  //printf("track_type : %d\n", track_type);
  //printf("ride_type : %d\n", rideType);
  //printf("rsubtype : %d\n", rideSubType);
//uint8_t map_x = (int) round(std::abs(env_actions_f[MAP_X]) * map_width) % map_width;
  //env_actions[MAP_X] = round(env_actions_f[MAP_X] * gMapSizeUnits);
  //uint8_t map_y = (int) round(std::abs(env_actions_f[MAP_Y]) * map_width) % map_width;
  //env_actions[MAP_Y] = round(env_actions_f[MAP_Y] * gMapSizeUnits);
  //uint8_t map_z = (int) (round(std::abs(env_actions_f[MAP_Z] * (MAXIMUM_LAND_HEIGHT - MINIMUM_LAND_HEIGHT))) + MINIMUM_LAND_HEIGHT) % MAXIMUM_LAND_HEIGHT;
  //env_actions[MAP_Z] = round(env_actions_f[MAP_Z] * gMapSizeUnits);
  //env_actions[ACTION] = round(env_actions_f[ACTION] * 2);
  //int32_t trackType_32 = round(env_actions_f[TRACK_TYPE] * 255);
  //int32_t trackType = std::abs(trackType_32 % 255);
  //env_actions[TRACK_TYPE] = round(env_actions_f[TRACK_TYPE]);
  //Direction direction = std::abs((int)round(env_actions_f[DIRECTION] * 4)) % 4;
  //env_actions[DIRECTION] = round(env_actions_f[DIRECTION]);

  //std::cout << unsigned(direction) << std::endl;
  //std::cout << env_actions << std::endl;
  //int* actions = agent.Step();

  //int32_t trackType = 76;
    int32_t brakeSpeed = 0;
    int32_t colour = rand() % 25;
    int32_t seatRotation = 4;
    int trackPlaceFlags = 0;
    int32_t liftHillAndAlternativeState = 0;
    bool fromTrackDesign = 0;
//int screen_x = this->uiContext->GetWidth();
//int screen_y = GetHeight();
//agent.Configure(screen_x, screen_y);
    if (!cold_open) {
        if (count == cold_open_steps + 1){
            window_close_all();
            auto rideCreateAction = RideCreateAction(rideType, rideSubType, 0, 0);
            auto result_ride = GameActions::Execute(&rideCreateAction);
          //auto rideSetAppearanceAction = RideSetAppearanceAction(_currentRideIndex, RideSetAppearanceType::TrackColourMain, 0, 0);
          //auto result_appearance = GameActions::Execute(&rideSetAppearanceAction);
          //auto rideSetStatusAction = RideSetStatusAction(_currentRideIndex, 1);
          //auto result_status = GameActions::Execute(&rideSetStatusAction);

          //CoordsXY loc = {x, y};
          //StationIndex stationNum = 0;
          //bool isExit = false;
          //auto rideEntrancePlaceAction = RideEntranceExitPlaceAction(loc, direction, _currentRideIndex, stationNum, isExit);
          //auto result_entrance = GameActions::Execute(&rideEntrancePlaceAction);
          //loc = {x, y};
          //stationNum = 0;
          //isExit = true;
          //auto rideExitPlaceAction = RideEntranceExitPlaceAction(loc, direction, _currentRideIndex, stationNum, isExit);
          //auto result_exit = GameActions::Execute(&rideExitPlaceAction);
          //_currentRideIndex++;
        }

      //std::cout << build_pos << std::endl;
      //build_trg = {10, 10, 30, direction};
        CoordsXYE next_pos;
      //track_type = TRACK_ELEM_FLAT;
//      track_type = TRACK_ELEM_RIGHT_QUARTER_TURN_5_TILES;
      //map_z = 112;
      //direction = 3;
        if (not one_build) {
          //map_x = 400;
          //map_y = 400;
            map_z = 7 * LAND_HEIGHT_STEP;
          //build_trg = {200, 200, 112, direction};
            build_trg = {map_x, map_y, map_z, track_direction};
        }
        else {
          //build_trg = {build_trg.x, build_trg.y, build_trg.z, build_trg.direction};
          //build_trg = {map_x, map_y, map_z, track_direction};
          //direction = _currentTrackPieceDirection;
          //track_type = _currentTrackPieceType;
          //printf("%d %d\n", next_pos.x, next_pos.y);
        }
        auto trackPlaceAction = TrackPlaceAction(_currentRideIndex, track_type, build_trg, brakeSpeed, colour, seatRotation, liftHillAndAlternativeState, fromTrackDesign);
      //trackPlaceAction.SetCallback(RideConstructPlacedForwardGameActionCallback);
      // _currentTrackSelectionFlags |= TRACK_SELECTION_FLAG_TRACK_PLACE_ACTION_QUEUED;
        auto result_track = GameActions::Execute(&trackPlaceAction);
        GA_ERROR success = result_track->Error;
        direction_int = track_direction;
        next_z = map_z;
        if (success == GA_ERROR::OK) {
            std::cout << "build success, step " << std::to_string(count) << " x " << build_trg.x << " y " << build_trg.y << " z " << build_trg.z << " track type " << to_string(track_type)<< " direction: " << to_string(build_trg.direction) << std::endl;
            if (one_build) {
                map_x = build_trg.x;
                map_y = build_trg.y;
                next_z = build_trg.z;
                direction_int = build_trg.direction;
              //map_x_grid = map_x / COORDS_XY_STEP;
              //map_y_grid = map_y / COORDS_XY_STEP;
            }
            torch::Tensor reward_deltas = torch::ones({{1}});
            TileElement* tile = map_get_first_element_at({ map_x, map_y });
            CoordsXYE start = {map_x, map_y, tile};
            CoordsXYE* start_p = &start;
            track_get_back(start_p, &next_pos); 
            Ride* ride = get_ride(_currentRideIndex);
          //int32_t next_direction;
          //int next_z;
            CoordsXYE first_pos = next_pos;
            bool first_iteration = false;
            track_block_get_next_from_zero(map_x, map_y, next_z, ride, direction_int, &next_pos, &next_z, &direction_int, false);
            do {
                map_x = next_pos.x;
                map_y = next_pos.y;
                map_x_grid = map_x / COORDS_XY_STEP;
                map_y_grid = map_y / COORDS_XY_STEP;
              //map_z = next_z;
                printf("new x: %d, y: %d, z: %d, d: %d\n", next_pos.x, next_pos.y, next_z, direction_int);
              //printf("map x and y coords: %d %d\n", map_x_grid, map_y_grid);
                if (!first_iteration && first_pos == next_pos) {
                    printf("looped!\n");
                    break;
                }
            } while (track_block_get_next(&next_pos, &next_pos, &next_z, &direction_int));
          //cout << tile << endl;

          //printf("next pos x, y, z, d: %d %d %d %d\n", map_x, map_y, next_z);
          //cout << "direction: " << direction_int << endl;
            build_trg = {next_pos.x, next_pos.y, next_z, direction_int};
          //build_trg = {map_x, map_y, map_z, track_direction};

            bool moveSlowIt = true;
            track_circuit_iterator it = {};
            track_circuit_iterator_begin(&it, next_pos); 
            track_circuit_iterator slowIt = it;
            while (track_circuit_iterator_next(&it))
            {
                reward_deltas[0] = reward_deltas[0] + 1;
                std::cout << "in circuit iterator, reward: " << reward_deltas[0] << std::endl;
                if (!track_is_connected_by_shape(it.last.element, it.current.element))
                {
                  //*output = it.current;
                    break;
                }
                //#2081: prevent an infinite loop
                moveSlowIt = !moveSlowIt;
                if (moveSlowIt)
                {
                    track_circuit_iterator_next(&slowIt);
                    if (track_circuit_iterators_match(&it, &slowIt))
                    {
                      //*output = it.current;
                      break;
                    }
                }
            }
            if (!it.looped)
            {
              //*output = it.last;
            }

          //printf("build pos %d %d %d", _currentTrackBegin.x, _currentTrackBegin.y, _currentTrackBegin.z);
            one_build = true;
          //gGotoStartPlacementMode = true;
          //ride_select_next_section();
          //ride_construction_set_default_next_piece();
          //ride_entrance_exit_place_provisional_ghost();
          //ride_entrance_exit_place_provisional_ghost();
          //std::cout << "tile->AsTrack() " << tile->AsTrack() << std::endl;
          //ride_clear_for_construction(ride);
          //CoordsXYE start_track;
          //auto ride_get_start_of_track(start_track);
          //printf("start: %d %d\n", start_track.x, start_track.y);
          //auto trackPlaceAction_2 = TrackPlaceAction(_currentRideIndex, track_type, build_trg, brakeSpeed, colour, seatRotation, liftHillAndAlternativeState, fromTrackDesign);
          //auto result_track_2 = GameActions::Execute(&trackPlaceAction_2);

          //printf("done step\n");
          //int rew_length = ride_get_total_length(ride);
          //printf("ride length: %d\n", rew_length);
          //printf("x %d y %d tt %d", map_x, map_y, track_type);
          //auto opts = torch::TensorOptions().dtype(torch::kInt);
          //std::cout << actions << std::endl;
          //printf("j_tt: %d", j_tt);
          //printf("%d, %d", map_x_grid, map_y_grid);
          //actions.resize(12);
          //torch::Tensor build_encoding = torch::ones({actions_len, 1, 1});
          // FIXME: turns into integers in state tensor, wtf
          //std::cout << actions << std::endl;
          //std::cout << actions_flat << std::endl;
            torch::Tensor build_encoding = torch::from_blob(actions.data(), {agent->n_action_bins}, torch::dtype(torch::kBool));
          //std::cout << build_encoding << std::endl;
            build_encoding.unsqueeze_(-1);
            build_encoding.unsqueeze_(-1);
          //std::cout << build_encoding.sizes() << std::endl;
          //std::cout << state.slice(0,0,12).slice(1, 3, 4).slice(2, 4, 5) << std::endl;
            state.slice(0, 0, n_chan).slice(1, map_x_grid-1, map_x_grid).slice(2, map_y_grid-1, map_y_grid) = build_encoding.slice(0, j_z, j_z + n_z_bins + n_tt_bins);
          //std::vector<float> reward_deltas = {1};
            rewards = rewards + reward_deltas;
          //std::cout << state << std::endl;
          //std::cout << build_encoding << std::endl;
          //std::cout << actions << std::endl;
        }
        else { // if failed
      //printf("failed build on step %d, x: %d, y %d, z: %d, track_type: %d\n", count, build_trg.x, build_trg.y, build_trg.z, track_type);
        }
          //std::cout << rewards[0][0] << std::endl;
      //rct_string_id ErrorTitle = result_track->ErrorTitle;
      //std::cout << std::to_string(ErrorTitle) << std::endl;
      //if (ErrorTitle != STR_RIDE_CONSTRUCTION_CANT_CONSTRUCT_THIS_HERE){
      //    reward += 1;
      //}
    }
    else if (count >= cold_open_steps) {
        cold_open = false;
    }
    else {  
        // here is where we would build stuff in advance of agent actions
    }
    count++;
  //if (count == max_step) {
  //  this->Reset();
  //}
    std::vector<std::vector<bool> > dones = {{0}};
    if (this->n_step == max_step) {
        dones[0][0] = 1;
        this->Reset();
    }
    StepResult step_result = {
        rewards,
        dones,
        Observe(),
    }; 
    return step_result;
}





