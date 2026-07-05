#pragma once

#include "core/ecs/entities/ui/ui_rect/ui_rect.hpp"
#include "core/types.hpp"

namespace atmo::core::components
{
    struct UIColorPicker {
        core::types::Color current_color = core::types::Color::WHITE;
    };
} // namespace atmo::core::components

template <> struct atmo::meta::ComponentMeta<atmo::core::components::UIColorPicker> {
    static constexpr const char *name = "UIColorPicker";
    static constexpr const char *category = "UI";
    static constexpr auto fields = std::make_tuple(atmo::meta::field<&atmo::core::components::UIColorPicker::current_color>("current_color").skipFlecs());
};

namespace atmo::core::ecs::entities
{
    constexpr std::string_view RowR = "Row R";
    constexpr std::string_view RowG = "Row G";
    constexpr std::string_view RowB = "Row B";
    constexpr std::string_view RowA = "Row A";

    class UIColorPicker : public EntityRegistry::Registrable<UIColorPicker, UIRect>
    {
    public:
        using EntityRegistry::Registrable<UIColorPicker, UIRect>::Registrable;

        static void RegisterSystems(flecs::world *world);

        void initialize();

        static constexpr std::string_view LocalName()
        {
            return "UIColorPicker";
        }

        void setColor(const types::Color &color);
        types::Color getColor() const;

        Clay_ElementDeclaration buildDecl() override;
        void draw(ClaySdL3RendererData *data) override;

    private:
        void setupRow(const std::string &label, const std::string &rowName);
        void syncRow(std::string_view rowName, float value);
    };
} // namespace atmo::core::ecs::entities
