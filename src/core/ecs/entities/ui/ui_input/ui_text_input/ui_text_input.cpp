#include "ui_text_input.hpp"
#include <string>
#include "core/ecs/entities/ui/ui_input/ui_input.hpp"
#include "meta/auto_register.hpp"

namespace atmo::core::ecs::entities
{
    void UITextInput::RegisterSystems(flecs::world *world)
    {
        world->system<core::components::UITextInput>("TextInput_Update").kind(flecs::OnStore).each([](flecs::entity e, core::components::UITextInput &comp) {
            if (comp.value == comp.prev_value)
                return;

            UITextInput input(e);
            if (input.isFocused())
                return;

            comp.prev_value = comp.value;
            input.setText(comp.value);
        });
    }

    void UITextInput::commit()
    {
        auto &comp = getComponentMutable<core::components::UITextInput>();
        auto text = getText();
        if (text == comp.value && comp.value == comp.prev_value)
            return;

        comp.value = text;
        comp.prev_value = text;
        getSignal<std::string>("StringValueChanged").emit(comp.value);
    }

    void UITextInput::clear()
    {
        UIInput::clear();
        auto &comp = getComponentMutable<core::components::UITextInput>();
        comp.value.clear();
        comp.prev_value.clear();
    }

    void UITextInput::setValue(const std::string &value)
    {
        auto &comp = getComponentMutable<core::components::UITextInput>();
        comp.value = value;
        comp.prev_value = value;
        setText(value);
    }

    void UITextInput::initialize()
    {
        UIInput::initialize();

        setComponent<core::components::UITextInput>({});
        createSignal<std::string>("StringValueChanged");

        getComponentMutable<core::components::UIInput>().input_type = core::components::UIInput::InputType::Text;
    }
} // namespace atmo::core::ecs::entities

ATMO_REGISTER_ENTITY(entities::UITextInput);
