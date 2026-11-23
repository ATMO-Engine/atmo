#include "file_dialog.hpp"
#include <exception>
#include <filesystem>
#include <memory>
#include <string>
#include <system_error>
#include "core/ecs/entities/entity.hpp"
#include "spdlog/spdlog.h"

namespace atmo::editor
{
    static void HandleFileDialogResult(void *userdata, const char *const *filelist, int /*filter*/)
    {
        std::shared_ptr<FileDialogCallback> callback(static_cast<FileDialogCallback *>(userdata));

        if (!filelist) {
            spdlog::error("File dialog error: {}", SDL_GetError());
            return;
        }
        if (!filelist[0])
            return;

        core::SignalQueue::Enqueue([callback, path = std::string(filelist[0])] {
            try {
                (*callback)(path);
            } catch (const std::exception &e) {
                spdlog::error("File dialog action failed: {}", e.what());
            }
        });
    }

    static std::string DialogLocation(const std::string &default_location)
    {
        std::error_code ec;
        if (std::filesystem::is_directory(default_location, ec))
            return (std::filesystem::path(default_location) / "").string();
        return default_location;
    }

    void ShowSaveFileDialog(FileDialogCallback callback, const std::string &default_location, std::span<const SDL_DialogFileFilter> filters)
    {
        auto *userdata = new FileDialogCallback(std::move(callback));
        const std::string location = DialogLocation(default_location);
        SDL_ShowSaveFileDialog(&HandleFileDialogResult, userdata, nullptr, filters.data(), static_cast<int>(filters.size()), location.c_str());
    }

    void ShowOpenFileDialog(FileDialogCallback callback, const std::string &default_location, std::span<const SDL_DialogFileFilter> filters)
    {
        auto *userdata = new FileDialogCallback(std::move(callback));
        const std::string location = DialogLocation(default_location);
        SDL_ShowOpenFileDialog(&HandleFileDialogResult, userdata, nullptr, filters.data(), static_cast<int>(filters.size()), location.c_str(), false);
    }
} // namespace atmo::editor
