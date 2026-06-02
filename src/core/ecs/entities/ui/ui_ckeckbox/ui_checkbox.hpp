#pragma once

#include <string>
#include "core/ecs/entities/ui/ui_rect/ui_rect.hpp"

namespace atmo::core::components
{
    struct UICheckBox {
        bool trigger = false;
    };
} // namespace atmo::core::components

namespace atmo::core::ecs::entities
{
    class UICheckBox : public EntityRegistry::Registrable<UICheckBox, UIRect>
    {
    public:
        using EntityRegistry::Registrable<UICheckBox, UIRect>::Registrable;

        static void RegisterSystems(flecs::world *world);

        void initialize();

        static constexpr std::string_view LocalName()
        {
            return "UICheckBox";
        }

        Clay_ElementDeclaration buildDecl() override;
        void draw(ClaySdL3RendererData *data) override;
    };
} // namespace atmo::core::ecs::entities
