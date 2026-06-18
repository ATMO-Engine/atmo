#pragma once

#include <string>
#include "core/ecs/entities/ui/ui_rect/ui_rect.hpp"

namespace atmo::core::components
{
    struct UIButton {
        bool is_hovered = false;
        bool is_pressed = false;
    };
} // namespace atmo::core::components

template <> struct atmo::meta::ComponentMeta<atmo::core::components::UIButton> {
    static constexpr const char *name = "Button";
    static constexpr const char *category = "UI";
    static constexpr auto fields = std::make_tuple();
};

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
