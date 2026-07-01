#pragma once

#include <regex>
#include <string>
#include "core/ecs/entities/ui/ui.hpp"
#include "meta/field_descriptor.hpp"

namespace atmo::core::components
{
    struct UIInput {
        std::string input_data;
        std::string prev_input_data;

        enum class InputType {
            Text,
            Int,
            Float
        };

        InputType input_type = InputType::Text;
        bool editing = false;
    };
} // namespace atmo::core::components

template <> struct atmo::meta::ComponentMeta<atmo::core::components::UIInput> {
    static constexpr const char *name = "UIInput";
    static constexpr const char *category = "UI";
    static constexpr auto fields = std::make_tuple(
        atmo::meta::field<&atmo::core::components::UIInput::input_data>("input_data"),
        atmo::meta::field<&atmo::core::components::UIInput::input_type>("input_type"), atmo::meta::field<&atmo::core::components::UIInput::editing>("editing"));
};

namespace atmo::core::ecs::entities
{
    class UIInput : public EntityRegistry::Registrable<UIInput, UI>
    {
    public:
        using EntityRegistry::Registrable<UIInput, UI>::Registrable;

        static void RegisterSystems(flecs::world *world);

        void initialize();

        virtual void validateInput() = 0;

        virtual void clear() = 0;

        void setValue();

        static constexpr std::string_view LocalName()
        {
            return "UIInput";
        }

        void draw(ClaySdL3RendererData *data) override;
    };
} // namespace atmo::core::ecs::entities
