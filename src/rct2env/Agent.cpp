#include "Agent.h"

using namespace OpenRCT2;

void Agent::Configure(int width, int height)
{
				screen_width = width + 1;
				screen_height = height + 1;
				
			}

int* Agent::Step()
{
        		num_guests = 0;
        		uint16_t spriteIndex;
        		Peep* peep;
        		FOR_ALL_PEEPS (spriteIndex, peep)
        			num_guests ++;
              //log_info(std::to_string(num_guests).c_str());
        	////screen_x = rand() % screen_width;
        	////screen_x = screen_x - 1;
        	////screen_y = rand() % screen_height;
        	////screen_y = screen_y - 1;
        		act_i = rand() % num_acts;
        		key_i = rand() % num_keys;
        		mouse_i = rand() % num_mouse;
        		build_i = rand() % num_builds;
        		subbuild_i = rand() % num_subbuilds;

				map_x = rand() % gMapSizeUnits;
				map_y = rand() % gMapSizeUnits;
                map_z = rand() % gMapSizeUnits;
                direction = rand() % 4;
                track_type = rand() % 256;
			  //key_i = 0;
		      //act_i = 1;
			  //mouse_i = 1;
			  //screen_x = 100;
			  //screen_y = 100;
			  //build_i = 50;
			  //subbuild_i = 50;
				
              //actions[1] = screen_x;
              //actions[2] = screen_y;
              //actions[3] = key_i;
              //actions[4] = mouse_i;
              //actions[5] = build_i;
              //actions[6] = subbuild_i;

                actions[ACTION] = act_i;
                actions[MAP_X] = map_x;
                actions[MAP_Y] = map_y;
                actions[MAP_Z] = map_z;
                actions[DIRECTION] = direction;
                actions[TRACK_TYPE] = track_type;

				return actions;
			}
