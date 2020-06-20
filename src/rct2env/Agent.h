#pragma once

#include "../openrct2/ui/UiContext.h"
#include "../openrct2-ui/input/KeyboardShortcuts.h"
#include "../openrct2/peep/Peep.h"
#include "../openrct2/world/Sprite.h"

			enum
			{
				ACTION,
				MAP_X,
				MAP_Y,
				MAP_Z,
				DIRECTION,
				TRACK_TYPE,
				// size
				NUM_ACTIONS,
			};
namespace OpenRCT2
{
	class Agent final {
		private:


			int actions [NUM_ACTIONS];
			int screen_x;
			int screen_y;
			int num_acts = 5;
			int num_keys = SHORTCUT_COUNT + 1;
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
			int screen_width;
			int screen_height;
			Agent() {};
			Agent(const Agent&) = delete;
			void Configure(int width, int height);
			int* Step(void);
	};
}
