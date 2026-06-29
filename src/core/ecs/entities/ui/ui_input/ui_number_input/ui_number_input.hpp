#pragma once

#include <variant>
#include "core/ecs/entities/ui/ui_input/ui_input.hpp"

namespace atmo::core::components
{
    struct UINumberInput {
        std::variant<int, float> value;
        std::variant<int, float> prev_value;
    };
} // namespace atmo::core::components

namespace atmo::core::ecs::entities
{
    class UINumberInput : public EntityRegistry::Registrable<UINumberInput, UIInput>
    {
    public:
        using EntityRegistry::Registrable<UINumberInput, UIInput>::Registrable;

        static void RegisterSystems(flecs::world *world);

        void initialize();

        virtual void validateInput() override;

        virtual void clear() override;

        static constexpr std::string_view LocalName()
        {
            return "UINumberInput";
        }

        Clay_ElementDeclaration buildDecl() override;
        void draw(ClaySdL3RendererData *data) override;
    };
} // namespace atmo::core::ecs::entities
