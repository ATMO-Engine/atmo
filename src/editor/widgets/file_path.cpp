#include "core/ecs/entities/entity.hpp"
#include "core/ecs/entities/ui/ui_input/ui_input.hpp"
#include "core/ecs/entities/ui/ui_input/ui_text_input/ui_text_input.hpp"
#include "core/ecs/entity_registry.hpp"
#include "meta/widget_registry.hpp"

std::optional<atmo::core::ecs::entities::Entity> createFilePathWidget(atmo::core::ecs::entities::Entity parent, void *value, const atmo::meta::FieldInfo &field)
{
    auto string_input_entity = atmo::core::ecs::EntityRegistry::Create<atmo::core::ecs::entities::UITextInput>("Entity::UI::UIInput::UITextInput");
    auto &input_entity_comp = string_input_entity->getComponentMutable<atmo::core::components::UIInput>();
    auto &string_input_entity_comp = string_input_entity->getComponentMutable<atmo::core::components::UITextInput>();

    input_entity_comp.input_type = atmo::core::components::UIInput::InputType::Text;
    if (value)
        field.get(value, &string_input_entity_comp.value);

    string_input_entity->getSignal<std::string>("StringValueChanged").connect([value, field](std::string val) { field.set(value, &val); });

    return *string_input_entity;
}

void updateFilePathWidget(atmo::core::ecs::entities::Entity widget, void *value, const atmo::meta::FieldInfo &field)
{
    auto &comp = widget.getComponentMutable<atmo::core::components::UITextInput>();
    field.get(value, &comp.value);
}

ATMO_REGISTER_WIDGET("file_path", createFilePathWidget, {}, updateFilePathWidget);
