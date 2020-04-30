#include "EnvContext.h"
#include "UiContext.h"
#include <openrct2-ui/audio/AudioContext.h>
#include "../openrct2-ui/drawing/BitmapReader.h"

#include <openrct2/Context.h>
#include <openrct2/OpenRCT2.h>
#include <openrct2/PlatformEnvironment.h>
#include <openrct2/audio/AudioContext.h>
#include <openrct2/cmdline/CommandLine.hpp>
#include <openrct2/platform/platform.h>
#include <openrct2/ui/UiContext.h>
#include "Env.h"
using namespace OpenRCT2;
using namespace OpenRCT2::Audio;
using namespace OpenRCT2::Ui;
using namespace cpprl;

RCT2Env::RCT2Env()
{
}
void RCT2Env::Init(int argc, const char** argv)
{
    std::unique_ptr<IContext> context;
    int32_t rc = EXIT_SUCCESS;
    int runGame = cmdline_run(argv, argc);
    core_init();
    RegisterBitmapReader();
    if (runGame == EXITCODE_CONTINUE)
    {
        if (gOpenRCT2Headless)
        {
            // Run OpenRCT2 with a plain context
            context = CreateContext();
        }
        else
        {
            // Run OpenRCT2 with a UI context
            auto env = to_shared(CreatePlatformEnvironment());
            auto audioContext = to_shared(CreateAudioContext());
            auto uiContext = to_shared(CreateUiContext(env));
            context = CreateContext(env, audioContext, uiContext);
        }
      //rc = context->RunOpenRCT2(argc, argv);
    }
    else if (runGame == EXITCODE_FAIL)
    {
        rc = EXIT_FAILURE;
    }
}

EnvInfo RCT2Env::GetInfo()
{
    EnvInfo env_info = EnvInfo();
    env_info.observation_space_type = "Discrete";
    return env_info;
}
