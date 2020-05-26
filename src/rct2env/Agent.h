#pragma once

#include "../openrct2/ui/UiContext.h"
#include "../openrct2-ui/input/KeyboardShortcuts.h"
#include "../openrct2/peep/Peep.h"
#include "../openrct2/world/Sprite.h"

namespace OpenRCT2
{
	class Agent final {
		private:
			int actions [5];
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
			int coord_x;
			int coord_y;
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
