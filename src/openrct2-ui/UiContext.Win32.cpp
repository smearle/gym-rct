/*****************************************************************************
 * Copyright (c) 2014-2019 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#ifdef _WIN32

#    if defined(__MINGW32__) && !defined(WINVER) && !defined(_WIN32_WINNT)
// 0x0600 == vista
#        define WINVER 0x0600
#        define _WIN32_WINNT 0x0600
#    endif // __MINGW32__

// Windows.h needs to be included first
#    include <windows.h>
#    undef CreateWindow

// Then the rest
#    include "UiContext.h"

#    include <SDL.h>
#    include <SDL_syswm.h>
#    include <algorithm>
#    include <openrct2/common.h>
#    include <openrct2/core/Path.hpp>
#    include <openrct2/core/String.hpp>
#    include <openrct2/ui/UiContext.h>
#    include <shlobj.h>
#    include <sstream>

// Native resource IDs
#    include "../../resources/resource.h"

static std::wstring SHGetPathFromIDListLongPath(LPCITEMIDLIST pidl)
{
    std::wstring pszPath(MAX_PATH, 0);
    while (!SHGetPathFromIDListW(pidl, &pszPath[0]))
    {
        if (pszPath.size() >= SHRT_MAX)
        {
            // Clearly not succeeding at all, bail
            return std::wstring();
        }
        pszPath.resize(pszPath.size() * 2);
    }
    return pszPath;
}

namespace OpenRCT2::Ui
{
    class Win32Context : public IPlatformUiContext
    {
    private:
        HMODULE _win32module;

    public:
        Win32Context()
        {
            _win32module = GetModuleHandleA(nullptr);
        }

        void SetWindowIcon(SDL_Window* window) override
        {
            if (_win32module != nullptr)
            {
                HICON icon = LoadIconA(_win32module, MAKEINTRESOURCEA(IDI_ICON));
                if (icon != nullptr)
                {
                    HWND hwnd = GetHWND(window);
                    if (hwnd != nullptr)
                    {
                        SendMessageA(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)icon);
                    }
                }
            }
        }

        bool IsSteamOverlayAttached() override
        {
            return (GetModuleHandleA("GameOverlayRenderer.dll") != nullptr);
        }

        void ShowMessageBox(SDL_Window* window, const std::string& message) override
        {
            HWND hwnd = GetHWND(window);
            std::wstring messageW = String::ToWideChar(message);
            MessageBoxW(hwnd, messageW.c_str(), L"OpenRCT2", MB_OK);
        }

        void OpenFolder(const std::string& path) override
        {
            std::wstring pathW = String::ToWideChar(path);
            ShellExecuteW(NULL, L"open", pathW.c_str(), NULL, NULL, SW_SHOWNORMAL);
        }

        std::string ShowFileDialog(SDL_Window* window, const FileDialogDesc& desc) override
        {
            std::wstring wcFilename = String::ToWideChar(desc.DefaultFilename);
            wcFilename.resize(std::max<size_t>(wcFilename.size(), MAX_PATH));

            std::wstring wcTitle = String::ToWideChar(desc.Title);
            std::wstring wcInitialDirectory = String::ToWideChar(desc.InitialDirectory);
            std::wstring wcFilters = GetFilterString(desc.Filters);

            // Set open file name options
            OPENFILENAMEW openFileName = {};
            openFileName.lStructSize = sizeof(OPENFILENAMEW);
            openFileName.lpstrTitle = wcTitle.c_str();
            openFileName.lpstrInitialDir = wcInitialDirectory.c_str();
            openFileName.lpstrFilter = wcFilters.c_str();
            openFileName.lpstrFile = &wcFilename[0];
            openFileName.nMaxFile = (DWORD)wcFilename.size();

            // Open dialog
            BOOL dialogResult = FALSE;
            DWORD commonFlags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_NOCHANGEDIR;
            if (desc.Type == FILE_DIALOG_TYPE::OPEN)
            {
                openFileName.Flags = commonFlags | OFN_NONETWORKBUTTON | OFN_FILEMUSTEXIST;
                dialogResult = GetOpenFileNameW(&openFileName);
            }
            else if (desc.Type == FILE_DIALOG_TYPE::SAVE)
            {
                openFileName.Flags = commonFlags | OFN_CREATEPROMPT | OFN_OVERWRITEPROMPT;
                dialogResult = GetSaveFileNameW(&openFileName);
            }

            std::string resultFilename;
            if (dialogResult)
            {
                resultFilename = String::ToUtf8(openFileName.lpstrFile);

                // If there is no extension, append the pattern
                std::string resultExtension = Path::GetExtension(resultFilename);
                if (resultExtension.empty())
                {
                    int32_t filterIndex = openFileName.nFilterIndex - 1;

                    assert(filterIndex >= 0);
                    assert(filterIndex < (int32_t)desc.Filters.size());

                    std::string pattern = desc.Filters[filterIndex].Pattern;
                    std::string patternExtension = Path::GetExtension(pattern);
                    if (!patternExtension.empty())
                    {
                        resultFilename += patternExtension;
                    }
                }
            }
            return resultFilename;
        }

        std::string ShowDirectoryDialog(SDL_Window* window, const std::string& title) override
        {
            std::string result;

            // Initialize COM and get a pointer to the shell memory allocator
            LPMALLOC lpMalloc;
            if (SUCCEEDED(CoInitializeEx(0, COINIT_APARTMENTTHREADED)) && SUCCEEDED(SHGetMalloc(&lpMalloc)))
            {
                std::wstring titleW = String::ToWideChar(title);
                BROWSEINFOW bi = {};
                bi.lpszTitle = titleW.c_str();
                bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE | BIF_NONEWFOLDERBUTTON;

                LPITEMIDLIST pidl = SHBrowseForFolderW(&bi);
                if (pidl != nullptr)
                {
                    result = String::ToUtf8(SHGetPathFromIDListLongPath(pidl));
                }
                CoTaskMemFree(pidl);
            }
            else
            {
                log_error("Error opening directory browse window");
            }
            CoUninitialize();

            // SHBrowseForFolderW might minimize the main window,
            // so make sure that it's visible again.
            ShowWindow(GetHWND(window), SW_RESTORE);

            return result;
        }

    private:
        HWND GetHWND(SDL_Window* window)
        {
            HWND result = nullptr;
            if (window != nullptr)
            {
                SDL_SysWMinfo wmInfo;
                SDL_VERSION(&wmInfo.version);
                if (SDL_GetWindowWMInfo(window, &wmInfo) != SDL_TRUE)
                {
                    log_error("SDL_GetWindowWMInfo failed %s", SDL_GetError());
                    exit(-1);
                }

                result = wmInfo.info.win.window;
            }
            return result;
        }

        static std::wstring GetFilterString(const std::vector<FileDialogDesc::Filter> filters)
        {
            std::wstringstream filtersb;
            for (auto filter : filters)
            {
                filtersb << String::ToWideChar(filter.Name) << '\0' << String::ToWideChar(filter.Pattern) << '\0';
            }
            return filtersb.str();
        }
    };

    IPlatformUiContext* CreatePlatformUiContext()
    {
        return new Win32Context();
    }
} // namespace OpenRCT2::Ui

#endif // _WIN32
