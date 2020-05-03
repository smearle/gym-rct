#include "EnvContext.h"
#include "../openrct2/Context.h"
namespace OpenRCT2
{
class EnvContext : public Context 
{
	public: 
		void RunGameLoop()
		{

            log_verbose("begin openrct2 loop");
            Context::_finished = false;

#ifndef __EMSCRIPTEN__
            Context::_variableFrame = Context::ShouldRunVariableFrame();
            do
            {
                Context::RunFrame();
                Context::_finished = true;
            } while (!Context::_finished);
#else
            emscripten_set_main_loop_arg(
                [](void* vctx) -> {
                    auto ctx = reinterpret_cast<Context*>(vctx);
                    ctx->RunFrame();
                },
                this, 0, 1);
#endif // __EMSCRIPTEN__
            log_verbose("finish openrct2 loop");
        }

		;
};
}
