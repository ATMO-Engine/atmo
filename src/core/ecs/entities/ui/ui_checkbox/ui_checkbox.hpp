#pragma once

#include <string>
#include "core/ecs/entities/ui/ui_rect/ui_rect.hpp"

namespace atmo::core::components
{
    struct UICheckBox {
        enum class CheckBoxState {
            IDLE,
            HOVER,
            PRESS
        };

        CheckBoxState state = UICheckBox::CheckBoxState::IDLE;
        bool trigger = true;
    };
} // namespace atmo::core::components

template <> struct atmo::meta::ComponentMeta<atmo::core::components::UICheckBox> {
    static constexpr const char *name = "CheckBox";
    static constexpr const char *category = "UI";
    static constexpr auto fields = std::make_tuple(
        atmo::meta::field<&atmo::core::components::UICheckBox::trigger>("trigger"),
        atmo::meta::field<&atmo::core::components::UICheckBox::state>("checkbox_state"));
};

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
