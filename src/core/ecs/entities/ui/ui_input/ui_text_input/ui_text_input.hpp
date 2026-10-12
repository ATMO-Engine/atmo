#pragma once

#include "core/ecs/entities/ui/ui_input/ui_input.hpp"

namespace atmo::core::components
{
    struct UITextInput {
        /// Last committed value. Writing it updates the displayed text unless the input is being edited.
        std::string value;
        std::string prev_value;
    };
} // namespace atmo::core::components

namespace atmo::core::ecs::entities
{
    /**
     * @brief Single-line string input. Emits `StringValueChanged(std::string)` when an edited value is committed.
     */
    class UITextInput : public EntityRegistry::Registrable<UITextInput, UIInput>
    {
    public:
        using EntityRegistry::Registrable<UITextInput, UIInput>::Registrable;

        static void RegisterSystems(flecs::world *world);

        void initialize();

        void commit() override;
        void clear() override;

        void setValue(const std::string &value);

        static constexpr std::string_view LocalName()
        {
            return "UITextInput";
        }
    };
} // namespace atmo::core::ecs::entities
