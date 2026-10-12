#pragma once

#include <string>
#include "core/ecs/entities/ui/ui_button/ui_button.hpp"
#include "core/ecs/entities/ui/ui_rect/ui_rect.hpp"

namespace atmo::core::components
{
    struct UICheckBox {
        bool default_texture = true;
    };
} // namespace atmo::core::components

template <> struct atmo::meta::ComponentMeta<atmo::core::components::UICheckBox> {
    static constexpr const char *name = "CheckBox";
    static constexpr const char *category = "UI";
    static constexpr auto fields = std::make_tuple(atmo::meta::field<&atmo::core::components::UICheckBox::default_texture>("default_texture"));
};

namespace atmo::core::ecs::entities
{
    class UICheckBox : public EntityRegistry::Registrable<UICheckBox, UIButton>
    {
    public:
        using EntityRegistry::Registrable<UICheckBox, UIButton>::Registrable;

        static void RegisterSystems(flecs::world *world);

        void initialize();

        static constexpr std::string_view LocalName()
        {
            return "UICheckBox";
        }

        void removeTexture();
        Clay_ElementDeclaration buildDecl() override;
        void draw(ClaySdL3RendererData *data) override;
    };
} // namespace atmo::core::ecs::entities
