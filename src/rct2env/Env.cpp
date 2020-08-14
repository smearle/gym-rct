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
//#include <spdlog/spdlog.h>
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

Agent agent = Agent();

RCT2Env::RCT2Env()
{
}


EnvInfo * RCT2Env::GetInfo()
{
    EnvInfo * env_info = new EnvInfo();
    env_info->observation_space_type = "Box";
    env_info->action_space_type = "MultiBinary";
    std::vector<long int> observation_space_shape = {n_chan, map_width, map_width};
  //Image image = get_observation();
  //std::vector<uint8_t> pixels = image.Pixels;
  //std::vector<long int> observation_space_shape = {pixels.size()};
  //std::vector<long int> observation_space_shape = {100};
    env_info->observation_space_shape = observation_space_shape;
    std::vector<long int>  action_space_shape = {agent.n_action_bins};
    env_info->action_space_shape = action_space_shape;
    return env_info;
}

void RCT2Env::Init(int argc, const char** argv)
{
  //std::unique_ptr<IContext> context;
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
    state = torch::zeros((n_chan, map_width, map_width));
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


StepResult RCT2Env::Step(std::vector<std::vector<float>> actions) {
  //spdlog::info("stepping env");
    this->Observe();
  //uint8_t* bits = drawing_engine_get_dpi()->bits;
  //spdlog::info( abi::__cxa_demangle(typeid(bits).name(), 0, 0, 0));
  //spdlog::info(&bits);
    this->context->RunFrame();
    rewards = torch::zeros({1, 1});
    std::vector<float> env_actions_f = actions[0];
    std::vector<int> env_actions(NUM_ACTION_TYPES);
    uint8_t map_x = (int) round(std::abs(env_actions_f[MAP_X]) * map_width) % map_width;
  //env_actions[MAP_X] = round(env_actions_f[MAP_X] * gMapSizeUnits);
    uint8_t map_y = (int) round(std::abs(env_actions_f[MAP_Y]) * map_width) % map_width;
  //env_actions[MAP_Y] = round(env_actions_f[MAP_Y] * gMapSizeUnits);
    uint8_t map_z = (int) (round(std::abs(env_actions_f[MAP_Z] * (MAXIMUM_LAND_HEIGHT - MINIMUM_LAND_HEIGHT))) + MINIMUM_LAND_HEIGHT) % MAXIMUM_LAND_HEIGHT;
  //env_actions[MAP_Z] = round(env_actions_f[MAP_Z] * gMapSizeUnits);
  //env_actions[ACTION] = round(env_actions_f[ACTION] * 2);
    int32_t trackType_32 = round(env_actions_f[TRACK_TYPE] * 255);
    int32_t trackType = std::abs(trackType_32 % 255);
  //env_actions[TRACK_TYPE] = round(env_actions_f[TRACK_TYPE]);
    Direction direction = std::abs((int)round(env_actions_f[DIRECTION] * 4)) % 4;
  //env_actions[DIRECTION] = round(env_actions_f[DIRECTION]);

    std::cout << unsigned(direction) << std::endl;
  //std::cout << env_actions << std::endl;
  //int* actions = agent.Step();
    rideType = RIDE_TYPE_CORKSCREW_ROLLER_COASTER;
    rideSubType = 4;
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
            auto trackPlaceAction = TrackPlaceAction(_currentRideIndex, trackType, origin, brakeSpeed, colour, seatRotation, liftHillAndAlternativeState, fromTrackDesign);
            auto result_track = GameActions::Execute(&trackPlaceAction);
            GA_ERROR success = result_track->Error;
            if (success == GA_ERROR::OK) {
              //std::cout << "build success, step " << std::to_string(count) << std::endl;
              //std::vector<float> reward_deltas = {1};
                torch::Tensor reward_deltas = torch::ones({{1}});
                rewards = rewards + reward_deltas;
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
    if (count == max_step) {
      this->Reset();
    }
    std::vector<std::vector<bool> > dones = {{false}};
    StepResult step_result = {
        rewards,
        dones,
        Observe(),
    }; 
    return step_result;
}

