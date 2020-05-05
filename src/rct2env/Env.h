#include "UiContext.h"
#include <cpprl/cpprl.h>

struct EnvInfo {
	std::string observation_space_type;
	std::string observation_space_shape;
	std::string action_space_type;
	std::string action_space_shape;
};

namespace OpenRCT2::Ui
{
	template<typename T> static std::shared_ptr<T> to_shared(std::unique_ptr<T>&& src)
	{
		return std::shared_ptr<T>(std::move(src));
	}

	class RCT2Env {
		private:
			std::unique_ptr<IContext> context;
		public:
			RCT2Env();	
	//RCT2Env(const RCT2Env&) = delete;
			void Init(int argc, const char** argv);
			EnvInfo GetInfo();
			void Step();
			torch::Tensor* Step(int* actions_tensor);
	};
}
