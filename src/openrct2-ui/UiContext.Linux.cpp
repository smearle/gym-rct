/*****************************************************************************
 * Copyright (c) 2014-2019 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#if (defined(__unix__) || defined(__EMSCRIPTEN__)) && !defined(__ANDROID__) && !defined(__APPLE__)

#    include "UiContext.h"

#    include <SDL.h>
#    include <dlfcn.h>
#    include <openrct2/common.h>
#    include <openrct2/core/Path.hpp>
#    include <openrct2/core/String.hpp>
#    include <openrct2/localisation/Localisation.h>
#    include <openrct2/ui/UiContext.h>
#    include <sstream>
#    include <stdexcept>

namespace OpenRCT2::Ui
{
    enum class DIALOG_TYPE
    {
        NONE,
        KDIALOG,
        ZENITY,
    };

    class LinuxContext final : public IPlatformUiContext
    {
    private:
    public:
        LinuxContext()
        {
        }

        void SetWindowIcon(SDL_Window* window) override
        {
        }

        bool IsSteamOverlayAttached() override
        {
#    ifdef __linux__
            // See http://syprog.blogspot.ru/2011/12/listing-loaded-shared-objects-in-linux.html
            struct lmap
            {
                void* base_address;
                char* path;
                void* unused;
                lmap* next;
                lmap* prev;
            };

            struct dummy
            {
                void* pointers[3];
                dummy* ptr;
            };

            bool result = false;
            void* processHandle = dlopen(nullptr, RTLD_NOW);
            if (processHandle != nullptr)
            {
                dummy* p = ((dummy*)processHandle)->ptr;
                lmap* pl = (lmap*)p->ptr;
                while (pl != nullptr)
                {
                    if (strstr(pl->path, "gameoverlayrenderer.so") != nullptr)
                    {
                        result = true;
                        break;
                    }
                    pl = pl->next;
                }
                dlclose(processHandle);
            }
            return result;
#    else
            return false; // Needed for OpenBSD, likely all other Unixes.
#    endif
        }

        void ShowMessageBox(SDL_Window* window, const std::string& message) override
        {
            log_verbose(message.c_str());

            std::string executablePath;
            DIALOG_TYPE dtype = GetDialogApp(&executablePath);

            switch (dtype)
            {
                case DIALOG_TYPE::KDIALOG:
                {
                    std::string cmd = String::Format(
                        "%s --title \"OpenRCT2\" --msgbox \"%s\"", executablePath.c_str(), message.c_str());
                    Execute(cmd);
                    break;
                }
                case DIALOG_TYPE::ZENITY:
                {
                    std::string cmd = String::Format(
                        "%s --title=\"OpenRCT2\" --info --text=\"%s\"", executablePath.c_str(), message.c_str());
                    Execute(cmd);
                    break;
                }
                default:
                    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_WARNING, "OpenRCT2", message.c_str(), window);
                    break;
            }
        }

        void OpenFolder(const std::string& path) override
        {
            std::string cmd = String::Format("xdg-open %s", EscapePathForShell(path).c_str());
            Execute(cmd);
        }

        std::string ShowFileDialog(SDL_Window* window, const FileDialogDesc& desc) override
        {
            std::string result;
            std::string executablePath;
            DIALOG_TYPE dtype = GetDialogApp(&executablePath);
            switch (dtype)
            {
                case DIALOG_TYPE::KDIALOG:
                {
                    std::string action = (desc.Type == FILE_DIALOG_TYPE::OPEN) ? "--getopenfilename" : "--getsavefilename";
                    std::string filter = GetKDialogFilterString(desc.Filters);
                    std::string cmd = String::StdFormat(
                        "%s --title '%s' %s '%s' '%s'", executablePath.c_str(), desc.Title.c_str(), action.c_str(),
                        desc.InitialDirectory.c_str(), filter.c_str());
                    std::string output;
                    if (Execute(cmd, &output) == 0)
                    {
                        result = output;
                    }
                    break;
                }
                case DIALOG_TYPE::ZENITY:
                {
                    std::string action = "--file-selection";
                    std::string flags;
                    if (desc.Type == FILE_DIALOG_TYPE::SAVE)
                    {
                        flags = "--confirm-overwrite --save";
                    }
                    std::string filters = GetZenityFilterString(desc.Filters);
                    std::string cmd = String::StdFormat(
                        "%s %s --filename='%s/' %s --title='%s' / %s", executablePath.c_str(), action.c_str(),
                        desc.InitialDirectory.c_str(), flags.c_str(), desc.Title.c_str(), filters.c_str());
                    std::string output;
                    if (Execute(cmd, &output) == 0)
                    {
                        if (desc.Type == FILE_DIALOG_TYPE::SAVE)
                        {
                            // The default file extension is taken from the **first** available filter, since
                            // we cannot obtain it from zenity's output. This means that the FileDialogDesc::Filters
                            // array must be carefully populated, at least the first element.
                            std::string pattern = desc.Filters[0].Pattern;
                            std::string defaultExtension = pattern.substr(pattern.find_last_of('.'));

                            const utf8* filename = Path::GetFileName(output.c_str());

                            // If there is no extension, append the pattern
                            const utf8* extension = Path::GetExtension(filename);
                            result = output;
                            if (extension[0] == '\0' && !defaultExtension.empty())
                            {
                                result = output.append(defaultExtension);
                            }
                        }
                        else
                        {
                            result = output;
                        }
                    }
                    break;
                }
                default:
                    ThrowMissingDialogApp();
                    break;
            }

            if (!result.empty())
            {
                if (desc.Type == FILE_DIALOG_TYPE::OPEN && access(result.c_str(), F_OK) == -1)
                {
                    std::string msg = String::StdFormat(
                        "\"%s\" not found: %s, please choose another file\n", result.c_str(), strerror(errno));
                    ShowMessageBox(window, msg);
                    return ShowFileDialog(window, desc);
                }
                else if (
                    desc.Type == FILE_DIALOG_TYPE::SAVE && access(result.c_str(), F_OK) != -1 && dtype == DIALOG_TYPE::KDIALOG)
                {
                    std::string cmd = String::StdFormat("%s --yesno \"Overwrite %s?\"", executablePath.c_str(), result.c_str());
                    if (Execute(cmd) != 0)
                    {
                        result = std::string();
                    }
                }
            }
            return result;
        }

        std::string ShowDirectoryDialog(SDL_Window* window, const std::string& title) override
        {
            std::string result;
            std::string executablePath;
            DIALOG_TYPE dtype = GetDialogApp(&executablePath);
            switch (dtype)
            {
                case DIALOG_TYPE::KDIALOG:
                {
                    std::string output;
                    std::string cmd = String::Format(
                        "%s --title '%s' --getexistingdirectory /", executablePath.c_str(), title.c_str());
                    if (Execute(cmd, &output) == 0)
                    {
                        result = output;
                    }
                    break;
                }
                case DIALOG_TYPE::ZENITY:
                {
                    std::string output;
                    std::string cmd = String::Format(
                        "%s --title='%s' --file-selection --directory /", executablePath.c_str(), title.c_str());
                    if (Execute(cmd, &output) == 0)
                    {
                        result = output;
                    }
                    break;
                }
                default:
                    ThrowMissingDialogApp();
                    break;
            }
            return result;
        }

    private:
        static DIALOG_TYPE GetDialogApp(std::string* executablePath)
        {
            // Prefer zenity as it offers more required features, e.g., overwrite
            // confirmation and selecting only existing files.
            // Silence error output with 2> /dev/null to avoid confusion in the
            // case where a user does not have zenity and/or kdialog.
            // OpenRCT2 will fall back to an SDL pop-up if the user has neither.
            if (Execute("which zenity 2> /dev/null", executablePath) == 0)
            {
                return DIALOG_TYPE::ZENITY;
            }
            if (Execute("which kdialog 2> /dev/null", executablePath) == 0)
            {
                return DIALOG_TYPE::KDIALOG;
            }
            return DIALOG_TYPE::NONE;
        }

        static int32_t Execute(const std::string& command, std::string* output = nullptr)
        {
#    ifndef __EMSCRIPTEN__
            log_verbose("executing \"%s\"...", command.c_str());
            FILE* fpipe = popen(command.c_str(), "r");
            if (fpipe == nullptr)
            {
                return -1;
            }

            if (output != nullptr)
            {
                // Read output into buffer
                std::vector<char> outputBuffer;
                char buffer[1024];
                size_t readBytes;
                while ((readBytes = fread(buffer, 1, sizeof(buffer), fpipe)) > 0)
                {
                    outputBuffer.insert(outputBuffer.begin(), buffer, buffer + readBytes);
                }

                // Trim line breaks
                size_t outputLength = outputBuffer.size();
                for (size_t i = outputLength - 1; i != SIZE_MAX; i--)
                {
                    if (outputBuffer[i] == '\n')
                    {
                        outputLength = i;
                    }
                    else
                    {
                        break;
                    }
                }

                // Convert to string
                *output = std::string(outputBuffer.data(), outputLength);
            }
            else
            {
                fflush(fpipe);
            }

            // Return exit code
            return pclose(fpipe);
#    else
            log_warning("Emscripten cannot execute processes. The commandline was '%s'.", command.c_str());
            return -1;
#    endif // __EMSCRIPTEN__
        }

        static std::string GetKDialogFilterString(const std::vector<FileDialogDesc::Filter> filters)
        {
            std::stringstream filtersb;
            bool first = true;
            for (const auto& filter : filters)
            {
                // KDialog wants filters space-delimited and we don't expect ';' anywhere else
                std::string pattern = filter.Pattern;
                for (size_t i = 0; i < pattern.size(); i++)
                {
                    if (pattern[i] == ';')
                    {
                        pattern[i] = ' ';
                    }
                }

                if (first)
                {
                    filtersb << String::StdFormat("%s | %s", pattern.c_str(), filter.Name.c_str());
                    first = false;
                }
                else
                {
                    filtersb << String::StdFormat("\\n%s | %s", pattern.c_str(), filter.Name.c_str());
                }
            }
            return filtersb.str();
        }

        static std::string GetZenityFilterString(const std::vector<FileDialogDesc::Filter> filters)
        {
            // Zenity seems to be case sensitive, while KDialog isn't
            std::stringstream filtersb;
            for (const auto& filter : filters)
            {
                filtersb << " --file-filter='" << filter.Name << " | ";
                for (char c : filter.Pattern)
                {
                    if (c == ';')
                    {
                        filtersb << ' ';
                    }
                    else if (isalpha(c))
                    {
                        filtersb << '[' << (char)tolower(c) << (char)toupper(c) << ']';
                    }
                    else
                    {
                        filtersb << c;
                    }
                }
                filtersb << "'";
            }
            return filtersb.str();
        }

        static void ThrowMissingDialogApp()
        {
            auto uiContext = GetContext()->GetUiContext();
            std::string dialogMissingWarning = language_get_string(STR_MISSING_DIALOG_APPLICATION_ERROR);
            uiContext->ShowMessageBox(dialogMissingWarning);
            throw std::runtime_error(dialogMissingWarning);
        }

        static std::string EscapePathForShell(std::string path)
        {
            for (size_t index = 0; (index = path.find('"', index)) != std::string::npos; index += 2)
            {
                path.replace(index, 1, "\\\"");
            }
            return '"' + path + '"';
        }
    };

    IPlatformUiContext* CreatePlatformUiContext()
    {
        return new LinuxContext();
    }
} // namespace OpenRCT2::Ui

#endif // (defined(__unix__) || defined(__EMSCRIPTEN__)) && !defined(__ANDROID__) && !defined(__APPLE__)
