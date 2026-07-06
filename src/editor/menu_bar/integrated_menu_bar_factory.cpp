#include "editor/menu_bar/i_platform_menu_bar.hpp"

#if !defined(ATMO_EXPORT) && !defined(__APPLE__)

#include "editor/menu_bar/clay_menu_bar.hpp"

namespace atmo::editor
{
    std::unique_ptr<IPlatformMenuBar> makePlatformMenuBar()
    {
        return std::make_unique<IntegratedMenuBar>();
    }
} // namespace atmo::editor

#endif
