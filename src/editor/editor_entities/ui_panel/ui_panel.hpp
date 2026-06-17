#pragma once

#include "core/ecs/entities/ui/ui_rect/ui_rect.hpp"

namespace atmo::core::components
{
    struct UIPanelState {
        bool open = false;
        bool is_dragging = false;
        float drag_offset_x = 0.0f;
    };
} // namespace atmo::core::components

template <> struct atmo::meta::ComponentMeta<atmo::core::components::UIPanelState> {
    static constexpr const char *name = "UIPanelState";
    static constexpr const char *category = "UI";
    static constexpr auto fields = std::make_tuple(
        atmo::meta::field<&atmo::core::components::UIPanelState::open>("open"),
        atmo::meta::field<&atmo::core::components::UIPanelState::is_dragging>("is_dragging"),
        atmo::meta::field<&atmo::core::components::UIPanelState::drag_offset_x>("drag_offset_x"));
};

namespace atmo::core::ecs::entities
{
    class UIPanel : public EntityRegistry::Registrable<UIPanel, UIRect>
    {
    public:
        using EntityRegistry::Registrable<UIPanel, UIRect>::Registrable;

        static void RegisterSystems(flecs::world *world);

        void initialize();

        static constexpr std::string_view LocalName()
        {
            return "UIPanel";
        }

        Clay_ElementDeclaration buildDecl() override;
        void draw(ClaySdL3RendererData *data) override;
    };
} // namespace atmo::core::ecs::entities
