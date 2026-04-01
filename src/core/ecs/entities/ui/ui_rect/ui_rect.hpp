#pragma once

#include <string>
#include "core/ecs/entities/ui/ui.hpp"
#include "core/types.hpp"
#include "meta/meta.hpp"

namespace atmo::core::components
{
    struct UIRect {
        struct CornerRadius {
            float top_left = 0.0f;
            float top_right = 0.0f;
            float bottom_left = 0.0f;
            float bottom_right = 0.0f;
        };

        struct Border {
            std::uint16_t left = 0;
            std::uint16_t right = 0;
            std::uint16_t top = 0;
            std::uint16_t bottom = 0;
            std::uint16_t between_children = 0;

            types::Color color = types::Color::WHITE;
        };

        CornerRadius corner_radius;
        Border border;
        types::Color color = types::Color::WHITE;
    };
} // namespace atmo::core::components

template <> struct atmo::meta::ComponentMeta<atmo::core::components::UIRect> {
    static constexpr const char *name = "Rect";
    static constexpr const char *category = "UI";
    static constexpr auto fields = std::make_tuple(
        atmo::meta::field<&atmo::core::components::UIRect::corner_radius>("corner_radius"),
        atmo::meta::field<&atmo::core::components::UIRect::border>("border"), atmo::meta::field<&atmo::core::components::UIRect::color>("color"));
};


namespace atmo::core::ecs::entities
{
    class UIRect : public EntityRegistry::Registrable<UIRect, UI>
    {
    public:
        using EntityRegistry::Registrable<UIRect, UI>::Registrable;

        static void RegisterSystems(flecs::world *world);

        void initialize();

        static constexpr std::string_view LocalName()
        {
            return "UIRect";
        }

        Clay_ElementDeclaration buildDecl() override;
        void draw() override;
    };
} // namespace atmo::core::ecs::entities
