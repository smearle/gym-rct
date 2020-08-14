#pragma once

//#include "../openrct2/ui/UiContext.h"
//#include "../openrct2-ui/input/KeyboardShortcuts.h"
#include "../openrct2/peep/Peep.h"
#include "../openrct2/world/Sprite.h"
#include <math.h>
#include <stdio.h>

			enum
			{
				ACTION,
				MAP_X,
				MAP_Y,
				MAP_Z,
				DIRECTION,
				TRACK_TYPE,
				// size
				NUM_ACTION_TYPES,
			};
namespace OpenRCT2
{
	class Agent final {
		private:


			int actions [NUM_ACTION_TYPES];
			int screen_x;
			int screen_y;
			int num_acts = 5;
//	int num_keys = SHORTCUT_COUNT + 1;
			int num_mouse = 9;
			int num_builds = 90;
			int num_subbuilds = 90;
			int act_i;
			int key_i;
			int build_i;
			int subbuild_i;
			int mouse_i;
			int map_x;
			int map_y;
			int map_z;
			int direction;
			int track_type;
			int num_guests;

		public:
			int n_actions_by_type[NUM_ACTION_TYPES] = {num_acts, gMapSizeUnits, gMapSizeUnits, gMapSizeUnits, 4, 256};
			int len_by_type[NUM_ACTION_TYPES];
			int count_n_bins ()
				{
				int n_action_bins = 0;
				for (int i = 0; i < NUM_ACTION_TYPES; i++) {
					int n_action_bins_i = ceil(log2(n_actions_by_type[i]));
					len_by_type[i] = n_action_bins_i;
					n_action_bins += n_action_bins_i;			
				}
				printf("%.1i", n_action_bins);
				return n_action_bins;
			}
			int n_action_bins = count_n_bins();
			int screen_width;
			int screen_height;
			Agent() {};
			Agent(const Agent&) = delete;
			void Configure(int width, int height);
			int* Step(void);
	};
}
