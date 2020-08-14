#include "Agent.h"
#include <unicode/uconfig.h>
#include <unicode/platform.h>
#include <unicode/unistr.h>
#include <openrct2/Context.h>
//#include "UiContext.h"
//#include <cpprl/cpprl.h>
#include <torch/torch.h>

//namespace rctai
struct EnvInfo {
	std::string observation_space_type;
	std::vector<long int> observation_space_shape;
    std::string action_space_type;
    std::vector<long int> action_space_shape;
};
struct StepResult {
	torch::Tensor rewards;
	std::vector<std::vector<bool> > done;
	torch::Tensor observation = torch::randn({100});
};

namespace OpenRCT2
{
	template<typename T> static std::shared_ptr<T> to_shared(std::unique_ptr<T>&& src)
	{
		return std::shared_ptr<T>(std::move(src));
	}
  //Agent agent = Agent();

	class RCT2Env {
		private:
            std::unique_ptr<IContext> context;
          //auto uiContext;
		    uint8_t map_width;
			uint8_t map_height = 50;
			// 8 for binary encoding of track type, 4 for directions
			uint8_t n_chan = 8 + 4;
            int act_i;
            int key_i;
            int rideType;
            int rideSubType;
            int count = 0;
			bool cold_open = false;
			int cold_open_steps = -1;
			torch::Tensor  rewards;
			int max_step = 500;
			std::string observation_space_type;
			std::vector<long int> observation_space_shape;
			// representation of game state for agent
			torch::Tensor state;
			std::string action_space_type;
			std::vector<int> action_space_shape;
        public:
			RCT2Env();	
	//RCT2Env(const RCT2Env&) = delete;
			void Init(int argc, const char** argv);
			EnvInfo * GetInfo();
			StepResult Step(std::vector<std::vector<bool>> actions);

////		std::vector<std::vector<float>> Observe();
////		std::vector<std::vector<float>> Reset();
			torch::Tensor Observe();
			torch::Tensor Reset();
	//torch::Tensor* Step(int* actions_tensor);

	};
}
