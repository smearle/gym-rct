/*****************************************************************************
 * Copyright (c) 2014-2019 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#if defined(__unix__) && !defined(__ANDROID__) && !defined(__APPLE__)

#    include <limits.h>
#    include <pwd.h>
#    include <vector>
#    if defined(__FreeBSD__) || defined(__NetBSD__)
#        include <stddef.h>
#        include <sys/param.h>
#        include <sys/sysctl.h>
#    endif // __FreeBSD__ || __NetBSD__
#    if defined(__linux__)
// for PATH_MAX
#        include <linux/limits.h>
#    endif // __linux__
#    include "../OpenRCT2.h"
#    include "../core/Path.hpp"
#    include "Platform2.h"
#    include "platform.h"

namespace Platform
{
    std::string GetFolderPath(SPECIAL_FOLDER folder)
    {
        switch (folder)
        {
            case SPECIAL_FOLDER::USER_CACHE:
            case SPECIAL_FOLDER::USER_CONFIG:
            case SPECIAL_FOLDER::USER_DATA:
            {
                auto path = GetEnvironmentPath("XDG_CONFIG_HOME");
                if (path.empty())
                {
                    auto home = GetFolderPath(SPECIAL_FOLDER::USER_HOME);
                    path = Path::Combine(home, ".config");
                }
                return path;
            }
            case SPECIAL_FOLDER::USER_HOME:
                return GetHomePath();
            default:
                return std::string();
        }
    }

    std::string GetDocsPath()
    {
        static const utf8* searchLocations[] = {
            "./doc",
            "/usr/share/doc/openrct2",
        };
        for (auto searchLocation : searchLocations)
        {
            log_verbose("Looking for OpenRCT2 doc path at %s", searchLocation);
            if (platform_directory_exists(searchLocation))
            {
                return searchLocation;
            }
        }
        return std::string();
    }

    static std::string GetCurrentWorkingDirectory()
    {
        char cwdPath[PATH_MAX];
        if (getcwd(cwdPath, sizeof(cwdPath)) != nullptr)
        {
            return cwdPath;
        }
        return std::string();
    }

    std::string GetInstallPath()
    {
        // 1. Try command line argument
        auto path = std::string(gCustomOpenrctDataPath);
        if (!path.empty())
        {
            return Path::GetAbsolute(path);
        }
        // 2. Try {${exeDir},${cwd},/}/{data,standard system app directories}
        // exeDir should come first to allow installing into build dir
        std::vector<std::string> prefixes;
        auto exePath = Platform::GetCurrentExecutablePath();
        auto exeDirectory = Path::GetDirectory(exePath);
        prefixes.push_back(exeDirectory);
        prefixes.push_back(GetCurrentWorkingDirectory());
        prefixes.push_back("/");
        static const char* SearchLocations[] = {
            "/data",
            "../share/openrct2",
#    ifdef ORCT2_RESOURCE_DIR
            // defined in CMakeLists.txt
            ORCT2_RESOURCE_DIR,
#    endif // ORCT2_RESOURCE_DIR
            "/usr/local/share/openrct2",
            "/var/lib/openrct2",
            "/usr/share/openrct2",
        };
        for (auto prefix : prefixes)
        {
            for (auto searchLocation : SearchLocations)
            {
                auto prefixedPath = Path::Combine(prefix, searchLocation);
                log_verbose("Looking for OpenRCT2 data in %s", prefixedPath.c_str());
                if (Path::DirectoryExists(prefixedPath))
                {
                    return prefixedPath;
                }
            }
        }
        return "/";
    }

    std::string GetCurrentExecutablePath()
    {
        char exePath[PATH_MAX] = { 0 };
#    ifdef __linux__
        auto bytesRead = readlink("/proc/self/exe", exePath, sizeof(exePath));
        if (bytesRead == -1)
        {
            log_fatal("failed to read /proc/self/exe");
        }
#    elif defined(__FreeBSD__) || defined(__NetBSD__)
#        if defined(__FreeBSD__)
        const int32_t mib[] = { CTL_KERN, KERN_PROC, KERN_PROC_PATHNAME, -1 };
#        else
        const int32_t mib[] = { CTL_KERN, KERN_PROC_ARGS, -1, KERN_PROC_PATHNAME };
#        endif
        auto exeLen = sizeof(exePath);
        if (sysctl(mib, 4, exePath, &exeLen, nullptr, 0) == -1)
        {
            log_fatal("failed to get process path");
        }
#    elif defined(__OpenBSD__)
        // There is no way to get the path name of a running executable.
        // If you are not using the port or package, you may have to change this line!
        strlcpy(exePath, "/usr/local/bin/", sizeof(exePath));
#    else
#        error "Platform does not support full path exe retrieval"
#    endif
        return exePath;
    }

    uintptr_t StrDecompToPrecomp(utf8* input)
    {
        return reinterpret_cast<uintptr_t>(input);
    }

    bool HandleSpecialCommandLineArgument(const char* argument)
    {
        return false;
    }
} // namespace Platform

#endif
