#include "core/ecs/entities/entity.hpp"
#include "core/ecs/entities/ui/ui_input/ui_input.hpp"
#include "core/ecs/entities/ui/ui_input/ui_number_input/ui_number_input.hpp"
#include "core/ecs/entity_registry.hpp"
#include "meta/widget_registry.hpp"

std::optional<atmo::core::ecs::entities::Entity> createIntWidget(atmo::core::ecs::entities::Entity parent, void *value, const atmo::meta::FieldInfo &field)
{
    auto int_input_entity = atmo::core::ecs::EntityRegistry::Create<atmo::core::ecs::entities::UINumberInput>("Entity::UI::UIInput::UINumberInput");
    auto &input_entity_comp = int_input_entity->getComponentMutable<atmo::core::components::UIInput>();
    auto &int_input_entity_comp = int_input_entity->getComponentMutable<atmo::core::components::UINumberInput>();

    input_entity_comp.input_type = atmo::core::components::UIInput::InputType::Int;
    field.get(value, &int_input_entity_comp.value);

    return *int_input_entity;
}

ATMO_REGISTER_WIDGET("int", createIntWidget, {})
