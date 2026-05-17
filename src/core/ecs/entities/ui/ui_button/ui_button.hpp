#pragma once

#include <string>
#include "core/ecs/entities/ui/ui_rect/ui_rect.hpp"

namespace atmo::core::ecs::entities
{
    class UIButton : public EntityRegistry::Registrable<UIButton, UIRect>
    {
    public:
        using EntityRegistry::Registrable<UIButton, UIRect>::Registrable;

        static void RegisterSystems(flecs::world *world);

        void initialize();

        static constexpr std::string_view LocalName()
        {
            return "UIButton";
        }

        Clay_ElementDeclaration buildDecl() override;
        void draw(ClaySdL3RendererData *data) override;
    };
} // namespace atmo::core::ecs::entities
