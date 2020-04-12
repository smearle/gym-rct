/*****************************************************************************
 * Copyright (c) 2014-2019 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#ifdef _WIN32

// Windows.h needs to be included first
#    include <windows.h>

// Then the rest
#    include <datetimeapi.h>
#    include <memory>
#    include <shlobj.h>
#    undef GetEnvironmentVariable

#    if !defined(__MINGW32__) && ((NTDDI_VERSION >= NTDDI_VISTA) && !defined(_USING_V110_SDK71_) && !defined(_ATL_XP_TARGETING))
#        define __USE_SHGETKNOWNFOLDERPATH__
#        define __USE_GETDATEFORMATEX__
#    else
#        ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#            define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#        endif
#    endif

#    include "../OpenRCT2.h"
#    include "../core/Path.hpp"
#    include "../core/String.hpp"
#    include "Platform2.h"
#    include "platform.h"

#    include <iterator>

namespace Platform
{
#    ifdef __USE_SHGETKNOWNFOLDERPATH__
    static std::string WIN32_GetKnownFolderPath(REFKNOWNFOLDERID rfid);
#    else
    static std::string WIN32_GetFolderPath(int nFolder);
#    endif
    static std::string WIN32_GetModuleFileNameW(HMODULE hModule);

    uint32_t GetTicks()
    {
        return platform_get_ticks();
    }

    std::string GetEnvironmentVariable(const std::string& name)
    {
        std::wstring result;
        auto wname = String::ToWideChar(name);
        wchar_t wvalue[256];
        auto valueSize = GetEnvironmentVariableW(wname.c_str(), wvalue, (DWORD)std::size(wvalue));
        if (valueSize < std::size(wvalue))
        {
            result = wvalue;
        }
        else
        {
            auto wlvalue = new wchar_t[valueSize];
            GetEnvironmentVariableW(wname.c_str(), wlvalue, valueSize);
            result = wlvalue;
            delete[] wlvalue;
        }
        return String::ToUtf8(result);
    }

    static std::string GetHomePathViaEnvironment()
    {
        std::string result;
        auto homedrive = GetEnvironmentVariable("HOMEDRIVE");
        auto homepath = GetEnvironmentVariable("HOMEPATH");
        if (!homedrive.empty() && !homepath.empty())
        {
            result = Path::Combine(homedrive, homepath);
        }
        return result;
    }

    std::string GetFolderPath(SPECIAL_FOLDER folder)
    {
        switch (folder)
        {
            // We currently store everything under Documents/OpenRCT2
            case SPECIAL_FOLDER::USER_CACHE:
            case SPECIAL_FOLDER::USER_CONFIG:
            case SPECIAL_FOLDER::USER_DATA:
            {
#    ifdef __USE_SHGETKNOWNFOLDERPATH__
                auto path = WIN32_GetKnownFolderPath(FOLDERID_Documents);
#    else
                auto path = WIN32_GetFolderPath(CSIDL_PERSONAL);
#    endif
                if (path.empty())
                {
                    path = GetFolderPath(SPECIAL_FOLDER::USER_HOME);
                }
                return path;
            }
            case SPECIAL_FOLDER::USER_HOME:
            {
#    ifdef __USE_SHGETKNOWNFOLDERPATH__
                auto path = WIN32_GetKnownFolderPath(FOLDERID_Profile);
#    else
                auto path = WIN32_GetFolderPath(CSIDL_PROFILE);
#    endif
                if (path.empty())
                {
                    path = GetHomePathViaEnvironment();
                    if (path.empty())
                    {
                        path = "C:\\";
                    }
                }
                return path;
            }
            case SPECIAL_FOLDER::RCT2_DISCORD:
            {
#    ifdef __USE_SHGETKNOWNFOLDERPATH__
                auto path = WIN32_GetKnownFolderPath(FOLDERID_LocalAppData);
#    else
                auto path = WIN32_GetFolderPath(CSIDL_LOCAL_APPDATA);
#    endif
                if (!path.empty())
                {
                    path = Path::Combine(path, "DiscordGames\\RollerCoaster Tycoon 2 Triple Thrill Pack\\content\\Game");
                }
                return path;
            }
            default:
                return std::string();
        }
    }

    static std::string GetCurrentExecutableDirectory()
    {
        auto exePath = GetCurrentExecutablePath();
        auto exeDirectory = Path::GetDirectory(exePath);
        return exeDirectory;
    }

    std::string GetInstallPath()
    {
        auto path = std::string(gCustomOpenrctDataPath);
        if (!path.empty())
        {
            path = Path::GetAbsolute(path);
        }
        else
        {
            auto exeDirectory = GetCurrentExecutableDirectory();
            path = Path::Combine(exeDirectory, "data");
        }
        return path;
    }

    std::string GetCurrentExecutablePath()
    {
        return WIN32_GetModuleFileNameW(nullptr);
    }

    std::string GetDocsPath()
    {
        return GetCurrentExecutableDirectory();
    }

    static SYSTEMTIME TimeToSystemTime(std::time_t timestamp)
    {
        LONGLONG ll = Int32x32To64(timestamp, 10000000) + 116444736000000000;

        FILETIME ft;
        ft.dwLowDateTime = (DWORD)ll;
        ft.dwHighDateTime = ll >> 32;

        SYSTEMTIME st;
        FileTimeToSystemTime(&ft, &st);
        return st;
    }

    std::string FormatShortDate(std::time_t timestamp)
    {
        SYSTEMTIME st = TimeToSystemTime(timestamp);

#    ifdef __USE_GETDATEFORMATEX__
        wchar_t date[20];
        GetDateFormatEx(LOCALE_NAME_USER_DEFAULT, DATE_SHORTDATE, &st, nullptr, date, (int)std::size(date), nullptr);
        std::string result = String::ToUtf8(std::wstring(date));
#    else
        char date[20];
        GetDateFormat(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &st, nullptr, date, sizeof(date));
        std::string result(date);
#    endif

        return result;
    }

    std::string FormatTime(std::time_t timestamp)
    {
        SYSTEMTIME st = TimeToSystemTime(timestamp);

#    ifdef __USE_GETDATEFORMATEX__
        wchar_t time[20];
        GetTimeFormatEx(LOCALE_NAME_USER_DEFAULT, 0, &st, nullptr, time, (int)std::size(time));
        std::string result = String::ToUtf8(std::wstring(time));
#    else
        char time[20];
        GetTimeFormat(LOCALE_USER_DEFAULT, 0, &st, nullptr, time, sizeof(time));
        std::string result(time);
#    endif

        return result;
    }

    bool IsOSVersionAtLeast(uint32_t major, uint32_t minor, uint32_t build)
    {
        bool result = false;
        auto hModule = GetModuleHandleA("ntdll.dll");
        if (hModule != nullptr)
        {
            using RtlGetVersionPtr = NTSTATUS(WINAPI*)(PRTL_OSVERSIONINFOW);
            auto fn = (RtlGetVersionPtr)GetProcAddress(hModule, "RtlGetVersion");
            if (fn != nullptr)
            {
                RTL_OSVERSIONINFOW rovi{};
                rovi.dwOSVersionInfoSize = sizeof(rovi);
                if (fn(&rovi) == 0)
                {
                    if (rovi.dwMajorVersion > major
                        || (rovi.dwMajorVersion == major
                            && (rovi.dwMinorVersion > minor || (rovi.dwMinorVersion == minor && rovi.dwBuildNumber >= build))))
                    {
                        result = true;
                    }
                }
            }
        }
        return result;
    }

    /**
     * Checks if the current version of Windows supports ANSI colour codes.
     * From Windows 10, build 10586 ANSI escape colour codes can be used on stdout.
     */
    static bool HasANSIColourSupport()
    {
        return IsOSVersionAtLeast(10, 0, 10586);
    }

    static void EnableANSIConsole()
    {
        if (HasANSIColourSupport())
        {
            auto handle = GetStdHandle(STD_OUTPUT_HANDLE);
            DWORD mode;
            GetConsoleMode(handle, &mode);
            if (!(mode & ENABLE_VIRTUAL_TERMINAL_PROCESSING))
            {
                mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
                SetConsoleMode(handle, mode);
            }
        }
    }

    bool IsColourTerminalSupported()
    {
        static bool hasChecked = false;
        static bool isSupported = false;
        if (!hasChecked)
        {
            if (HasANSIColourSupport())
            {
                EnableANSIConsole();
                isSupported = true;
            }
            else
            {
                isSupported = false;
            }
            hasChecked = true;
        }
        return isSupported;
    }

#    ifdef __USE_SHGETKNOWNFOLDERPATH__
    static std::string WIN32_GetKnownFolderPath(REFKNOWNFOLDERID rfid)
    {
        std::string path;
        wchar_t* wpath = nullptr;
        if (SUCCEEDED(SHGetKnownFolderPath(rfid, KF_FLAG_CREATE, nullptr, &wpath)))
        {
            path = String::ToUtf8(std::wstring(wpath));
        }
        CoTaskMemFree(wpath);
        return path;
    }
#    else
    static std::string WIN32_GetFolderPath(int nFolder)
    {
        std::string path;
        wchar_t wpath[MAX_PATH];
        if (SUCCEEDED(SHGetFolderPathW(nullptr, nFolder | CSIDL_FLAG_CREATE, nullptr, 0, wpath)))
        {
            path = String::ToUtf8(std::wstring(wpath));
        }
        return path;
    }
#    endif

    static std::string WIN32_GetModuleFileNameW(HMODULE hModule)
    {
        uint32_t wExePathCapacity = MAX_PATH;
        std::unique_ptr<wchar_t[]> wExePath;
        uint32_t size;
        do
        {
            wExePathCapacity *= 2;
            wExePath = std::make_unique<wchar_t[]>(wExePathCapacity);
            size = GetModuleFileNameW(hModule, wExePath.get(), wExePathCapacity);
        } while (size >= wExePathCapacity);
        return String::ToUtf8(wExePath.get());
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
