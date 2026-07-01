#pragma once

#include <variant>
#include "clay.h"
#include "core/ecs/entities/ui/ui_rect/ui_rect.hpp"

namespace atmo::core::components
{
    struct UISlider {
        enum class SliderType {
            Int,
            Float
        };

        SliderType type = SliderType::Float;
        std::variant<int, float> value = 0.0f;
        std::variant<int, float> min = 0.0f;
        std::variant<int, float> max = 1.0f;
        bool dragging = false;
        Clay_BoundingBox bounds = {};
    };
} // namespace atmo::core::components

template <> struct atmo::meta::ComponentMeta<atmo::core::components::UISlider> {
    static constexpr const char *name = "UISlider";
    static constexpr const char *category = "UI";
    static constexpr auto fields = std::make_tuple(
        atmo::meta::field<&atmo::core::components::UISlider::type>("type"), atmo::meta::field<&atmo::core::components::UISlider::value>("value").skipFlecs(),
        atmo::meta::field<&atmo::core::components::UISlider::min>("min").skipFlecs(),
        atmo::meta::field<&atmo::core::components::UISlider::max>("max").skipFlecs());
};

namespace atmo::core::ecs::entities
{
    class UISlider : public EntityRegistry::Registrable<UISlider, UIRect>
    {
    public:
        using EntityRegistry::Registrable<UISlider, UIRect>::Registrable;

        static void RegisterSystems(flecs::world *world);
        static void Unregister(flecs::world *world);

        void initialize();

        static constexpr std::string_view LocalName()
        {
            return "UISlider";
        }

        void setType(components::UISlider::SliderType type, float min, float max);
        void setValue(float value, bool triggerSignal = true);
        float getValue() const;

        Clay_ElementDeclaration buildDecl() override;
        void draw(ClaySdL3RendererData *data) override;
    };
} // namespace atmo::core::ecs::entities
