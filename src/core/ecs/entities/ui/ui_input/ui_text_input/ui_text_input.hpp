#pragma once

#include "core/ecs/entities/ui/ui_input/ui_input.hpp"

namespace atmo::core::components
{
    struct UITextInput {
        std::string value;
        std::string prev_value;
    };
} // namespace atmo::core::components

namespace atmo::core::ecs::entities
{
    class UITextInput : public EntityRegistry::Registrable<UITextInput, UIInput>
    {
    public:
        using EntityRegistry::Registrable<UITextInput, UIInput>::Registrable;

        static void RegisterSystems(flecs::world *world);

        void initialize();

        virtual void validateInput() override;

        virtual void clear() override;

        static constexpr std::string_view LocalName()
        {
            return "UITextInput";
        }

        Clay_ElementDeclaration buildDecl() override;
        void draw(ClaySdL3RendererData *data) override;
    };
} // namespace atmo::core::ecs::entities
