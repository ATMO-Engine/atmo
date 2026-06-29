#include <string>
#include "core/ecs/entities/entity.hpp"
#include "core/ecs/entities/ui/ui_input/ui_input.hpp"
#include "core/ecs/entities/ui/ui_input/ui_text_input/ui_text_input.hpp"
#include "core/ecs/entity_registry.hpp"
#include "meta/widget_registry.hpp"

std::optional<atmo::core::ecs::entities::Entity> createStringWidget(atmo::core::ecs::entities::Entity parent, void *value, const atmo::meta::FieldInfo &field)
{
    auto text_input_entity = atmo::core::ecs::EntityRegistry::Create<atmo::core::ecs::entities::UITextInput>("Entity::UI::UIInput::UITextInput");
    auto &input_entity_comp = text_input_entity->getComponentMutable<atmo::core::components::UIInput>();
    auto &text_input_entity_comp = text_input_entity->getComponentMutable<atmo::core::components::UITextInput>();

    if (value)
        field.get(value, &text_input_entity_comp.value);

    text_input_entity->getSignal<std::string>("StringValueChanged").connect([value, field](std::string val) { field.set(value, &val); });

    return *text_input_entity;
}

void updateStringWidget(atmo::core::ecs::entities::Entity widget, void *value, const atmo::meta::FieldInfo &field)
{
    auto &comp = widget.getComponentMutable<atmo::core::components::UITextInput>();
    field.get(value, &comp.value);
}

ATMO_REGISTER_WIDGET("string", createStringWidget, {}, updateStringWidget);
