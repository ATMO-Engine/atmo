#pragma once

#include <variant>
#include "core/ecs/entities/ui/ui_input/ui_input.hpp"

namespace atmo::core::components
{
    struct UINumberInput {
        /// Last committed value. Writing it updates the displayed text unless the input is being edited.
        std::variant<int, float> value;
        std::variant<int, float> prev_value;
    };
} // namespace atmo::core::components

namespace atmo::core::ecs::entities
{
    /**
     * @brief Numeric input. Only accepts numeric characters; invalid input reverts to the last valid value on commit.
     *
     * Emits `IntValueChanged(int)` or `FloatValueChanged(float)` depending on `UIInput::input_type`.
     */
    class UINumberInput : public EntityRegistry::Registrable<UINumberInput, UIInput>
    {
    public:
        using EntityRegistry::Registrable<UINumberInput, UIInput>::Registrable;

        static void RegisterSystems(flecs::world *world);

        void initialize();

        void commit() override;
        void clear() override;

        void setNumber(int value);
        void setNumber(float value);

        static constexpr std::string_view LocalName()
        {
            return "UINumberInput";
        }

    private:
        /// Converts `value` to the current input type and shows it.
        void showValue();
    };
} // namespace atmo::core::ecs::entities
