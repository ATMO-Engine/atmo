// Factory implementation for non-macOS platforms.
// On macOS, macos_menu_bar.mm provides this function instead.

#include "editor/menu_bar/i_platform_menu_bar.hpp"

#if !defined(ATMO_EXPORT) && !defined(__APPLE__)

#include "editor/menu_bar/clay_menu_bar.hpp"

namespace atmo::editor
{
    std::unique_ptr<IPlatformMenuBar> makePlatformMenuBar()
    {
        return std::make_unique<ClayMenuBar>();
    }
} // namespace atmo::editor

#endif
