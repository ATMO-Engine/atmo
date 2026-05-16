#pragma once

#include "editor/menu_bar/i_platform_menu_bar.hpp"

#if !defined(ATMO_EXPORT)

namespace atmo::editor
{
    class MacOSMenuBar : public IPlatformMenuBar
    {
    public:
        MacOSMenuBar();
        ~MacOSMenuBar() override;

        void build(core::ecs::entities::Entity window_entity, Commands &commands) override;

    private:
        struct Impl;
        Impl *m_impl = nullptr;
    };
} // namespace atmo::editor

#endif
