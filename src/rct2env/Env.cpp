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

#include <openrct2/actions/GameAction.h>
#include <openrct2/actions/RideCreateAction.hpp>
#include <openrct2/actions/TrackPlaceAction.hpp>
#include <openrct2/actions/RideSetAppearanceAction.hpp>
#include <openrct2/actions/RideSetStatus.hpp>
#include <openrct2/actions/RideEntranceExitPlaceAction.hpp>
#include <openrct2/world/Location.hpp>



//AGENT
#include <rct2env/Agent.h>
#include <openrct2/ride/Ride.h>
#include <openrct2/interface/Window.h>
#include <openrct2/interface/Cursors.h>
//END AGENT

using namespace OpenRCT2;
using namespace OpenRCT2::Audio;
using namespace OpenRCT2::Ui;
using namespace cpprl;

RCT2Env::RCT2Env()
{
}


EnvInfo RCT2Env::GetInfo()
{
    EnvInfo env_info = EnvInfo();
    env_info.observation_space_type = "Box";
    env_info.action_space_type = "Discrete";
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
		spdlog::info("Creating context:");
        if (gOpenRCT2Headless)
        {
			spdlog::info("plain context");
            // Run OpenRCT2 with a plain context
            context = CreateContext();
        }
        else
        {
			spdlog::info("ui context");
            // Run OpenRCT2 with a UI context
            auto env = to_shared(CreatePlatformEnvironment());
            auto audioContext = to_shared(CreateAudioContext());
            auto uiContext = to_shared(CreateUiContext(env));
            context = CreateContext(env, audioContext, uiContext);
        }
      rc = context->RunOpenRCT2(argc, argv);


    // Agent
    Agent agent = Agent();
    int mouse_i;

    }
    else if (runGame == EXITCODE_FAIL)
    {
        rc = EXIT_FAILURE;
    }
}

void RCT2Env::Observe() {
  //Image image = get_observation();
  //spdlog::info(typeid(image.Pixels).name());
  //std::vector<uint8_t> pixels = image.Pixels;
  //for (int i = 0; i < pixels.size(); i++) {
  //    std::cout << unsigned(pixels.at(i)) << ' ';
  //}
  //std::cout << pixels.size();
}



void RCT2Env::Step() {
  //spdlog::info("stepping env");
    this->Observe();
  //uint8_t* bits = drawing_engine_get_dpi()->bits;
  //spdlog::info( abi::__cxa_demangle(typeid(bits).name(), 0, 0, 0));
  //spdlog::info(&bits);
    this->context->RunFrame();

        //AGENT
      //int screen_x = this->uiContext->GetWidth();
      //int screen_y = GetHeight();
      //agent.Configure(screen_x, screen_y);
        int* actions = agent.Step();

        int x = 100;
        int y = 100;
        int z = 120;
        Direction direction = 0;
        CoordsXYZD origin = {x, y, z, direction};
        int32_t trackType = 76;
        int32_t brakeSpeed = 0;
        int32_t colour = 0;
        int32_t seatRotation = 4;
        int trackPlaceFlags = 0;
        int32_t liftHillAndAlternativeState = 0;
        bool fromTrackDesign = 0;
        origin = {actions[MAP_X], actions[MAP_Y], actions[MAP_Z], actions[DIRECTION]};
        auto trackPlaceAction = TrackPlaceAction(_currentRideIndex, actions[TRACK_TYPE], origin, brakeSpeed, colour, seatRotation, liftHillAndAlternativeState, fromTrackDesign);
        auto result_track = GameActions::Execute(&trackPlaceAction);




      //screen_x = actions[1];
      //screen_y = actions[2];
 
      //ScreenCoordsXY screenCoords = ScreenCoordsXY({screen_x, screen_y});
      //context_set_cursor_position(screenCoords);
      //_cursorState.position = screenCoords;
      //if (mouse_i == 1) {
      //    _cursorState.left = 1;
      //};
      //key_i = actions[3];
      //mouse_i = actions[4];
      //rideType = actions[5];
      //rideSubType = actions[6];

        rideType = RIDE_TYPE_CORKSCREW_ROLLER_COASTER;
        rideSubType = 4;
       

      //int x = 100;
      //int y = 100;
      //int z = 120;
      //Direction direction = 0;
      //CoordsXYZD origin = {x, y, z, direction};
      //int32_t trackType = 76;
      //int32_t brakeSpeed = 0;
      //int32_t colour = 0;
      //int32_t seatRotation = 4;
      //int trackPlaceFlags = 0;
      //int32_t liftHillAndAlternativeState = 0;
      //bool fromTrackDesign = 0;
      //origin = {actions[MAP_X], actions[MAP_Y], actions[MAP_Z], actions[DIRECTION]};
      //auto trackPlaceAction = TrackPlaceAction(_currentRideIndex, actions[TRACK_TYPE], origin, brakeSpeed, colour, seatRotation, liftHillAndAlternativeState, fromTrackDesign);
      //auto result_track = GameActions::Execute(&trackPlaceAction);
        if (count == 0){
    	////track_design_file_ref* = GetItemsForObjectEntry(rideType, entry);
    	////window_track_place_open(track_design_file_ref);
    		auto rideCreateAction = RideCreateAction(rideType, rideSubType, 0, 0);
    		auto result_ride = GameActions::Execute(&rideCreateAction);
    		auto trackPlaceAction = TrackPlaceAction(_currentRideIndex, trackType, origin, brakeSpeed, colour, seatRotation, liftHillAndAlternativeState, fromTrackDesign);
    		auto result_track = GameActions::Execute(&trackPlaceAction);
    	  //auto rideSetAppearanceAction = RideSetAppearanceAction(_currentRideIndex, RideSetAppearanceType::TrackColourMain, 0, 0);
    	  //auto result_appearance = GameActions::Execute(&rideSetAppearanceAction);
    		auto rideSetStatusAction = RideSetStatusAction(_currentRideIndex, 1);
    		auto result_status = GameActions::Execute(&rideSetStatusAction);
    //window_ride_main_open(get_ride(_currentRideIndex));
//  		_currentRideIndex++;
            CoordsXY loc = {x, y};
            StationIndex stationNum = 0;
            bool isExit = false;
            auto rideEntrancePlaceAction = RideEntranceExitPlaceAction(loc, direction, _currentRideIndex, stationNum, isExit);
            auto result_entrance = GameActions::Execute(&rideEntrancePlaceAction);
            loc = {x, y};
            stationNum = 0;
            isExit = true;
            auto rideExitPlaceAction = RideEntranceExitPlaceAction(loc, direction, _currentRideIndex, stationNum, isExit);
            auto result_exit = GameActions::Execute(&rideExitPlaceAction);
          //_currentRideIndex++;
        }
        if (count == 1 & false) {
            _currentRideIndex++;
        	x = 200;
        	y = 200;
        	rideType = 52;
        	rideSubType = 255;
			seatRotation = 0;
        	auto rideCreateAction = RideCreateAction(rideType, rideSubType, 0, 0);
        	auto result_ride = GameActions::Execute(&rideCreateAction);
        	trackType = 1;
			int x = 300;
			int y = 288;
			int z = 112;
			int tileStep = 32;
			Direction direction = 0;
			CoordsXYZD origin = {x, y, z, direction};
			for (int i = 0; i < 3; i++) {
				origin = {x, y, z, direction};
				auto trackPlaceAction = TrackPlaceAction(_currentRideIndex, trackType, origin, brakeSpeed, colour, seatRotation, liftHillAndAlternativeState, fromTrackDesign);
				auto result_track = GameActions::Execute(&trackPlaceAction);
                rct_string_id error_message = result_track->ErrorMessage;
                std::cout << error_message << std::endl;
                rct_string_id error_title = result_track->ErrorTitle;
                std::cout << error_title << std::endl;
				x = x + tileStep;
			}
			trackType = 0;
			liftHillAndAlternativeState = 1;
			origin = {x, y, z, direction};
			auto trackPlaceAction = TrackPlaceAction(_currentRideIndex, trackType, origin, brakeSpeed, colour, seatRotation, liftHillAndAlternativeState, fromTrackDesign);
			auto result_track = GameActions::Execute(&trackPlaceAction);
            rct_string_id error_message = result_track->ErrorMessage;
            std::cout << error_message << std::endl;
			x = x - tileStep;
			// turns
			trackType = 42;
			origin = {x, y, z, direction};
			auto trackPlaceAction1 = TrackPlaceAction(_currentRideIndex, trackType, origin, brakeSpeed, colour, seatRotation, liftHillAndAlternativeState, fromTrackDesign);
			result_track = GameActions::Execute(&trackPlaceAction1);
            error_message = result_track->ErrorMessage;
            std::cout << error_message << std::endl;
			x = x - tileStep;
			y = y - 2 * tileStep;
			direction = 3;
			origin = {x, y, z, direction};
			auto trackPlaceAction2 = TrackPlaceAction(_currentRideIndex, trackType, origin, brakeSpeed, colour, seatRotation, liftHillAndAlternativeState, fromTrackDesign);
			result_track = GameActions::Execute(&trackPlaceAction2);
            error_message = result_track->ErrorMessage;
            std::cout << error_message << std::endl;

			x = x + 2 * tileStep;
			y = y - tileStep;

			for (int i = 0; i < 3; i++) {
				trackType = 0;
				direction = 2;
				origin = {x, y, z, direction};
				auto trackPlaceAction = TrackPlaceAction(_currentRideIndex, trackType, origin, brakeSpeed, colour, seatRotation, liftHillAndAlternativeState, fromTrackDesign);
				auto result_track = GameActions::Execute(&trackPlaceAction);
				x = x + tileStep;
              //std::cout << result_track->Position.x << std::endl;
              //std::cout << result_track->Position.y << std::endl;
              //std::cout << result_track->Position.z << std::endl;
                rct_string_id error_message = result_track->ErrorMessage;
                std::cout << error_message << std::endl;
		//liftHillAndAlternativeState = 1;
			}
			// turns
			trackType = 42;
			origin = {x, y, z, direction};
			for (int i = 0; i < 1; i++){
			auto trackPlaceAction1 = TrackPlaceAction(_currentRideIndex, trackType, origin, brakeSpeed, colour, seatRotation, liftHillAndAlternativeState, fromTrackDesign);
			auto result_track1 = GameActions::Execute(&trackPlaceAction1);
			x = x + tileStep;
			y = y + 2 * tileStep;
			direction = 1;
			origin = {x, y, z, direction};
			auto trackPlaceAction2 = TrackPlaceAction(_currentRideIndex, trackType, origin, brakeSpeed, colour, seatRotation, liftHillAndAlternativeState, fromTrackDesign);
			auto result_track2 = GameActions::Execute(&trackPlaceAction2);
			x = x - 2 * tileStep;
	        y = y + 2 * tileStep;
            CoordsXY loc = {x, y};
            direction = 3;
            StationIndex stationNum = 0;
            bool isExit = false;
            auto rideEntrancePlaceAction = RideEntranceExitPlaceAction(loc, direction, _currentRideIndex, stationNum, isExit);
            auto result_entrance = GameActions::Execute(&rideEntrancePlaceAction);
			x = x - tileStep;
            loc = {x, y};
            stationNum = 0;
            isExit = true;
            auto rideExitPlaceAction = RideEntranceExitPlaceAction(loc, direction, _currentRideIndex, stationNum, isExit);
            auto result_exit = GameActions::Execute(&rideExitPlaceAction);
    		auto rideSetStatusAction = RideSetStatusAction(_currentRideIndex, 1);
    		auto result_status = GameActions::Execute(&rideSetStatusAction);
			}
        }

    count++;
}

