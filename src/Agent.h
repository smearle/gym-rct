#include "openrct2/ui/UiContext.h"
#include "openrct2-ui/input/KeyboardShortcuts.h"

namespace OpenRCT2
{
	class Agent final {
		private:
			int actions [5];
			int screen_x;
			int screen_y;
			int num_acts = 3;
			int num_keys = SHORTCUT_COUNT;
			int num_mouse = 90;
			int act_i;
			int key_i;
			int mouse_i;
		public:
			int screen_width;
			int screen_height;
			Agent() {};
			Agent(const Agent&) = delete;
			void Configure(int width, int height) {
				screen_width = width;
				screen_height = height;
			}
			int* Step(void){
				screen_x = rand() % screen_width;
				screen_y = rand() % screen_height;
				act_i = rand() % num_acts;
				key_i = rand() % num_keys;
				mouse_i = rand() % num_mouse;
				actions[0] = act_i;		
				actions[1] = screen_x;
				actions[2] = screen_y;
				actions[3] = key_i;
				actions[4] = mouse_i;
				return actions;
			};
	};
}
