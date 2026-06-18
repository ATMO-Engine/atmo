#pragma once

#include "core/ecs/entities/ui/ui_rect/ui_rect.hpp"

namespace atmo::core::ecs::entities
{
    class UIPopup : public EntityRegistry::Registrable<UIPopup, UIRect>
    {
    public:
        using EntityRegistry::Registrable<UIPopup, UIRect>::Registrable;

        static void RegisterSystems(flecs::world *world);

        void initialize();

        static constexpr std::string_view LocalName()
        {
            return "UIPopup";
        }

        Clay_ElementDeclaration buildDecl() override;
        void draw(ClaySdL3RendererData *data) override;
    };
} // namespace atmo::core::ecs::entities
