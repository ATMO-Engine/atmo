#pragma once

#include <functional>
#include <span>
#include <string>
#include "SDL3/SDL_dialog.h"

namespace atmo::editor
{
    using FileDialogCallback = std::function<void(const std::string &)>;

    /**
     * @brief Open the OS "save file" dialog. @p callback is called with the chosen path, and not called if the user cancels.
     *
     * @param callback Called with the selected path.
     * @param default_location Folder (or file) the dialog starts in.
     * @param filters Optional file type filters.
     */
    void ShowSaveFileDialog(FileDialogCallback callback, const std::string &default_location, std::span<const SDL_DialogFileFilter> filters = {});

    /**
     * @brief Open the OS "open file" dialog. @p callback is called with the chosen path, and not called if the user cancels.
     *
     * @param callback Called with the selected path.
     * @param default_location Folder the dialog starts in.
     * @param filters Optional file type filters.
     */
    void ShowOpenFileDialog(FileDialogCallback callback, const std::string &default_location, std::span<const SDL_DialogFileFilter> filters = {});
} // namespace atmo::editor
