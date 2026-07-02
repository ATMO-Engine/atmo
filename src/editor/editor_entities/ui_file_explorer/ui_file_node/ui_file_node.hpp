#pragma once

#include <flecs.h>
#include <string>
#include "core/ecs/entities/ui/ui_button/ui_button.hpp"

namespace atmo::core::ecs::entities
{
    class UIFileExplorerFileNode : public EntityRegistry::Registrable<UIFileExplorerFileNode, UIButton>
    {
    public:
        using EntityRegistry::Registrable<UIFileExplorerFileNode, UIButton>::Registrable;

        static void RegisterSystems(flecs::world *world);

        void initialize();

        static constexpr std::string_view LocalName()
        {
            return "UIFileExplorerFileNode";
        }

        void setPath(const std::string &path);

        void setHighlight(bool highlighted);
    };
} // namespace atmo::core::ecs::entities
