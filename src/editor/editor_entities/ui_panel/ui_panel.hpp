#pragma once

#include "core/ecs/entities/ui/ui_rect/ui_rect.hpp"

namespace atmo::core::components
{
    struct UIFloatingWindowState {
        bool is_dragging = false;
        float drag_offset_x = 0.0f;
    };
} // namespace atmo::core::components

template <> struct atmo::meta::ComponentMeta<atmo::core::components::UIFloatingWindowState> {
    static constexpr const char *name = "UIFloatingWindowState";
    static constexpr const char *category = "UI";
    static constexpr auto fields = std::make_tuple(
        atmo::meta::field<&atmo::core::components::UIFloatingWindowState::is_dragging>("is_dragging"),
        atmo::meta::field<&atmo::core::components::UIFloatingWindowState::drag_offset_x>("drag_offset_x"));
};

namespace atmo::core::ecs::entities
{
    class UIFloatingWindow : public EntityRegistry::Registrable<UIFloatingWindow, UIRect>
    {
    public:
        using EntityRegistry::Registrable<UIFloatingWindow, UIRect>::Registrable;

        static void RegisterSystems(flecs::world *world);

        void initialize();

        static constexpr std::string_view LocalName()
        {
            return "UIFloatingWindow";
        }

        Clay_ElementDeclaration buildDecl() override;
        void draw(ClaySdL3RendererData *data) override;
    };
} // namespace atmo::core::ecs::entities
