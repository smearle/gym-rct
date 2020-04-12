/*****************************************************************************
 * Copyright (c) 2014-2019 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include <openrct2/Context.h>
#include <openrct2/OpenRCT2.h>
#include <openrct2/cmdline/CommandLine.hpp>
#include <openrct2/platform/platform.h>

using namespace OpenRCT2;

/**
 * Main entry point for non-Windows sytems. Windows instead uses its own DLL proxy.
 */
int main(int argc, const char** argv)
{
    int32_t rc = EXIT_SUCCESS;
    int runGame = cmdline_run(argv, argc);
    core_init();
    if (runGame == EXITCODE_CONTINUE)
    {
        gOpenRCT2Headless = true;
        gOpenRCT2NoGraphics = true;

        // Run OpenRCT2 with a plain context
        auto context = CreateContext();
        rc = context->RunOpenRCT2(argc, argv);
    }
    else if (runGame == EXITCODE_FAIL)
    {
        rc = EXIT_FAILURE;
    }
    return rc;
}
