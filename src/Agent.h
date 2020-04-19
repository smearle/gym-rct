#include "openrct2/ui/UiContext.h"
#include "openrct2-ui/input/KeyboardShortcuts.h"
#include "openrct2/peep/Peep.h"
#include "openrct2/world/Sprite.h"

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
			int num_guests;
		public:
			int screen_width;
			int screen_height;
			Agent() {};
			Agent(const Agent&) = delete;
			void Configure(int width, int height) {
				screen_width = width + 1;
				screen_height = height + 1;
			}
			int* Step(void){
				num_guests = 0;
				uint16_t spriteIndex;
				Peep* peep;
				FOR_ALL_PEEPS (spriteIndex, peep)
					num_guests ++;
		      //log_info(std::to_string(num_guests).c_str());
				screen_x = rand() % screen_width;
				screen_x = screen_x - 1;
				screen_y = rand() % screen_height;
				screen_y = screen_y - 1;
				act_i = rand() % num_acts;
				key_i = rand() % num_keys;
				mouse_i = rand() % num_mouse;
				build_i = rand() % num_builds;
				subbuild_i = rand() % num_subbuilds;
			    key_i = 0;
		      //act_i = 1;
				mouse_i = 1;
			    screen_x = -1;
			    screen_y = -1;
				build_i = 9;
				subbuild_i = 10;
				
				actions[0] = act_i;		
				actions[1] = screen_x;
				actions[2] = screen_y;
				actions[3] = key_i;
				actions[4] = mouse_i;
				actions[5] = build_i;
				actions[6] = subbuild_i;
				return actions;
			};
	};
}
