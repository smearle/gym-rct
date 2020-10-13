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
    this->agent->Configure(map_width, map_width);
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
    int mouse_i;
    state = torch::zeros({n_chan, map_width, map_width}).to(torch::kBool);
    }
    else if (runGame == EXITCODE_FAIL)
    {
        rc = EXIT_FAILURE;
    }
}
torch::Tensor RCT2Env::Reset() {
    prev_success = false;
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
    int j = *bin_j;
    for (int i = 0; i < n_bins; i++) {
        bool act_b = actions[0][i + j];
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
    switch(track_type) {
        case 205:
            track_type = 0;
            break;
    }
    track_type = track_type % (TRACK_MINI_GOLF_HOLE - 1);
    if ((50 <= track_type) && (track_type <= 53)) {
        track_type -= 3;
    }
//  track_type = TRACK_NONE;
    map_z_grid = 7;
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

        CoordsXYE next_pos;
      //track_type = TRACK_ELEM_FLAT;
        build_trg = {map_x, map_y, map_z, track_direction};
        if (not one_build) {
            // If we haven't build anything
            map_z = 7 * LAND_HEIGHT_STEP;
        }
        else if (prev_success) {
            build_trg = {build_trg.x, build_trg.y, build_trg.z, build_trg.direction};
          //build_trg = {map_x, map_y, map_z, track_direction};
          //direction = _currentTrackPieceDirection;
          //track_type = _currentTrackPieceType;
        }
        auto trackPlaceAction = TrackPlaceAction(_currentRideIndex, track_type, build_trg, brakeSpeed, colour, seatRotation, liftHillAndAlternativeState, fromTrackDesign);
      // _currentTrackSelectionFlags |= TRACK_SELECTION_FLAG_TRACK_PLACE_ACTION_QUEUED;
        auto result_track = GameActions::Execute(&trackPlaceAction);
        GA_ERROR success = result_track->Error;
        direction_int = track_direction;
        next_z = map_z;
        if (success == GA_ERROR::OK) {
            prev_success = true;
          //std::cout << "build success, step " << std::to_string(count) << " x " << build_trg.x << " y " << build_trg.y << " z " << build_trg.z << " track type " << to_string(track_type)<< " direction: " << to_string(build_trg.direction) << std::endl;
            if (one_build) {
              //map_x = build_trg.x;
              //map_y = build_trg.y;
              //next_z = build_trg.z;
              //direction_int = build_trg.direction;
              //map_x_grid = map_x / COORDS_XY_STEP;
              //map_y_grid = map_y / COORDS_XY_STEP;
            }
            torch::Tensor reward_deltas = torch::zeros({{1}}) + 0.1;
            TileElement* tile = map_get_first_element_at({ build_trg.x, build_trg.y });
            CoordsXYE start = {build_trg.x, build_trg.y, tile};
            CoordsXYE* start_p = &start;
            track_get_back(start_p, &next_pos); 
            Ride* ride = get_ride(_currentRideIndex);
            CoordsXYE first_pos = next_pos;
            bool first_iteration = false;
            track_block_get_next_from_zero(build_trg.x, build_trg.y, next_z, ride, direction_int, &next_pos, &next_z, &direction_int, false);
            // TODO: do this backward to get continuity score!
            int seq_len = 0;
            int rew = 0;
            do {
                if (seq_len > 0) {
                    printf("getting next build position, %d iterations\n", seq_len);
                    rew += 5;
                }
                seq_len += 1;
              //map_x = next_pos.x;
              //map_y = next_pos.y;
              //map_x_grid = map_x / COORDS_XY_STEP;
              //map_y_grid = map_y / COORDS_XY_STEP;
              //map_z = next_z;
              //printf("new x: %d, y: %d, z: %d, d: %d\n", next_pos.x, next_pos.y, next_z, direction_int);
              //printf("map x and y coords: %d %d\n", map_x_grid, map_y_grid);
                if (!first_iteration && first_pos == next_pos) {
                    printf("looped!\n");
                    break;
                }
            } while (track_block_get_next(&next_pos, &next_pos, &next_z, &direction_int));


         //CoordsXYE last_pos = next_pos;
         //track_get_back(start_p, &last_pos); 
         //int last_z;
         //int last_d;
         //track_block_get_previous_from_zero(last_pos.x, last_pos.y, next_z, ride, direction_int, &last_pos, &last_z, &last_d, false);
         //seq_len = 0;
         //do {
         //     if (seq_len > 0) {
         //         printf("moving back through track, %d iterations\n", seq_len);
         //     }
         //     seq_len += 1;
         //     rew += 5;
         //   //map_x = next_pos.x;
         //   //map_y = next_pos.y;
         //   //map_x_grid = map_x / COORDS_XY_STEP;
         //   //map_y_grid = map_y / COORDS_XY_STEP;
         //   //map_z = next_z;
         //   //printf("new x: %d, y: %d, z: %d, d: %d\n", next_pos.x, next_pos.y, next_z, direction_int);
         //   //printf("map x and y coords: %d %d\n", map_x_grid, map_y_grid);
         //     if (!first_iteration && first_pos == next_pos) {
         //         printf("looped!\n");
         //         break;
         //     }
         // } while (track_block_get_previous(&last_pos, &last_pos, &last_z, &last_d));

          //cout << tile << endl;

          //printf("next pos x, y, z, d: %d %d %d %d\n", map_x, map_y, next_z);
          //cout << "direction: " << direction_int << endl;

            bool moveSlowIt = true;
            track_circuit_iterator it = {};
            track_circuit_iterator_begin(&it, first_pos); 
            track_circuit_iterator slowIt = it;
            seq_len = 0;
          //while (track_circuit_iterator_next(&it))
          //{
          //    seq_len += 1;
          //    rew += 20;
          //    cout << "in circuit iterator, seq len : " << seq_len << endl;
          //    if (!track_is_connected_by_shape(it.last.element, it.current.element))
          //    {
          //      //*output = it.current;
          //        printf("bad connection\n");
          //        rew -= 10; 
          //      //break;
          //    }
          //    //#2081: prevent an infinite loop
          //    moveSlowIt = !moveSlowIt;
          //    if (moveSlowIt)
          //    {
          //        track_circuit_iterator_next(&slowIt);
          //        if (track_circuit_iterators_match(&it, &slowIt))
          //        {
          //          //*output = it.current;
          //          break;
          //        }
          //    }
          //}
          //if (!it.looped)
          //{
          //    if (seq_len > 0) { 
          //    cout << "looped in iterator" << endl;
          //  //*output = it.last;
          //    }
          //}
            reward_deltas[0] += rew;


            one_build = true;

            torch::Tensor build_encoding = torch::from_blob(actions.data(), {agent->n_action_bins}, torch::dtype(torch::kBool));
          //std::cout << build_encoding << std::endl;
            build_encoding.unsqueeze_(-1);
            build_encoding.unsqueeze_(-1);
          //std::cout << build_encoding.sizes() << std::endl;
          //std::cout << state.slice(0,0,12).slice(1, 3, 4).slice(2, 4, 5) << std::endl;
            state.slice(0, 0, n_chan).slice(1, build_trg.x-1, build_trg.x).slice(2, build_trg.y-1, build_trg.y) = build_encoding.slice(0, j_z, j_z + n_z_bins + n_tt_bins);
          //std::vector<float> reward_deltas = {1};
            rewards = rewards + reward_deltas;
          //std::cout << state << std::endl;
          //cout << build_encoding << endl;
          //std::cout << actions << std::endl;
            build_trg = {next_pos.x, next_pos.y, next_z, direction_int};
        }
        else { // if failed
            prev_success = false;
        //printf("failed build on step %d, x: %d, y %d, z: %d, track_type: %d\n", count, build_trg.x, build_trg.y, build_trg.z, track_type);
          //rct_string_id ErrorTitle = result_track->ErrorTitle;
          //std::cout << std::to_string(ErrorTitle) << std::endl;
          //if (ErrorTitle != STR_RIDE_CONSTRUCTION_CANT_CONSTRUCT_THIS_HERE){
          //    reward += 1;
          //}
        }
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





