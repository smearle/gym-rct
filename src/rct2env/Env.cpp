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
#include <openrct2/ride/TrackData.h>

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
    if (runGame == EXITCODE_CONTINUE)
    {
//		spdlog::info("Creating context:");
        if (gOpenRCT2Headless)
        {
			//spdlog::info("plain context");
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
    auto rideDemolishAction = RideDemolishAction(_currentRideIndex, 0);
    auto rideDemolishResult = GameActions::Execute(&rideDemolishAction);
    count = 0;
    state = torch::zeros({n_chan, map_width, map_width}).to(torch::kBool);
    this-> n_step = 0;
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


StepResult RCT2Env::Step(std::vector<std::vector<bool>> actions) {
    this->n_step += 1;
//  spdlog::info("stepping env");
  //for (int i = 0; i < 43; i++) {
  // // std::cout << actions.at(0).at(i) << ' ';
  //}
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

    int map_x_bins = agent->len_by_type[MAP_X];
  //printf("map_x_bins : %d\n", map_x_bins);
    uint8_t map_x_grid = 0;
    uint j = 0;
    for (int i = 0; i < map_x_bins + 2; i++) {
        map_x_grid = map_x_grid + actions[0][i] * pow(2, i);
        j += 1;
    }

    uint8_t map_y_bins =  agent->len_by_type[MAP_Y];
    uint8_t map_y_grid = 0;
    for (int i = 0; i < map_y_bins; i++) {
        map_y_grid = map_y_grid + actions[0][i + j] * pow(2, i);
        j += 1;
    }
    uint8_t map_z_bins =  agent->len_by_type[MAP_Z];
    uint8_t map_z = 0;
    for (int i = 0; i < map_z_bins; i++) {
        map_z = map_z + actions[0][i + j] * pow(2, i);
        j += 1;
    }
    uint8_t track_type_bins =  agent->len_by_type[TRACK_TYPE];
    uint8_t track_type = 0;
    for (int i = 0; i < 9; i++) {
        track_type += actions[0][i] * pow(2, i);
        j += 1;
    }
    int actions_len = map_x_bins + map_y_bins  + map_z_bins + track_type_bins;
    Direction direction = 0;

    uint8_t map_x = map_x_grid;
    uint8_t map_y = map_y_grid;
    map_x = map_x * COORDS_XY_STEP; 
    map_y = map_y * COORDS_XY_STEP; 
    map_z = 32;
  //map_z = map_z * LAND_HEIGHT_STEP; 

    rideType = RIDE_TYPE_CORKSCREW_ROLLER_COASTER;
    rideSubType = 4;
  //map_x = rand() % gMapSizeUnits;
  //map_y = rand() % gMapSizeUnits;
  //map_z = rand() % gMapSizeUnits;
  //direction = rand() % 4;
  //map_y = rand() % gMapSizeUnits;
  //map_z = rand() % gMapSizeUnits;

  //track_type = rand() % 256;
  //printf("map_x_bins : %d\n", map_x_bins);
  //printf("map_y_bins : %d\n", map_y_bins);
  //printf("map_z_bins : %d\n", map_z_bins);
  //printf("map_x : %d\n", map_x);
  //printf("map_y : %d\n", map_y);
  //printf("map_z : %d\n", map_z);
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

    int x = 100;
    int y = 100;
    int z = 120;
  //Direction direction = 0;
    CoordsXYZD origin = {x, y, z, direction};
  //int32_t trackType = 76;
    int32_t brakeSpeed = 0;
    int32_t colour = 0;
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

        origin = {map_x, map_y, map_z, direction};
        auto trackPlaceAction = TrackPlaceAction(_currentRideIndex, track_type, origin, brakeSpeed, colour, seatRotation, liftHillAndAlternativeState, fromTrackDesign);
        auto result_track = GameActions::Execute(&trackPlaceAction);
        GA_ERROR success = result_track->Error;
        if (success == GA_ERROR::OK) {
          //std::cout << "build success, step " << std::to_string(count) << std::endl;
            auto opts = torch::TensorOptions().dtype(torch::kBool);
          //std::cout << actions << std::endl;
            printf("%d, %d", map_x_grid, map_y_grid);
            actions.resize(12);
            torch::Tensor build_encoding = torch::from_blob(actions[:, 0, 0].data(), {actions_len}, opts).to(torch::kBool);
            build_encoding.unsqueeze_(-1);
            build_encoding.unsqueeze_(-1);
          //std::cout << build_encoding.sizes() << std::endl;
          //torch::Tensor build_encoding = torch::ones({12, 1, 1});
          //std::cout << state.slice(0,0,12).slice(1, 3, 4).slice(2, 4, 5) << std::endl;
            state.slice(0, 0, 12).slice(1, map_x_grid-1, map_x_grid).slice(2, map_y_grid-1, map_y_grid) = build_encoding.slice(0, map_x_bins + map_y_bins, map_x_bins + map_y_bins + 12);
          //std::vector<float> reward_deltas = {1};
            torch::Tensor reward_deltas = torch::ones({{1}}) * 100;
            rewards = rewards + reward_deltas;
            std::cout << state << std::endl;
            std::cout << build_encoding << std::endl;
            std::cout << actions << std::endl;
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

