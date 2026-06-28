#include "core/ecs/entities/entity.hpp"
#include "core/ecs/entities/ui/ui.hpp"
#include "core/ecs/entities/ui/ui_input/ui_input.hpp"
#include "core/ecs/entities/ui/ui_input/ui_number_input/ui_number_input.hpp"
#include "core/ecs/entity_registry.hpp"
#include "core/types.hpp"
#include "meta/widget_registry.hpp"
#include "spdlog/spdlog.h"


std::optional<atmo::core::ecs::entities::Entity> createVector2Widget(atmo::core::ecs::entities::Entity parent, void *value, const atmo::meta::FieldInfo &field)
{
    auto vector_container = atmo::core::ecs::EntityRegistry::Create<atmo::core::ecs::entities::UI>("Entity::UI");
    auto x_input_entity = atmo::core::ecs::EntityRegistry::Create<atmo::core::ecs::entities::UINumberInput>("Entity::UI::UIInput::UINumberInput");
    auto &input_entity_comp_x = x_input_entity->getComponentMutable<atmo::core::components::UIInput>();
    auto &x_input_entity_comp = x_input_entity->getComponentMutable<atmo::core::components::UINumberInput>();

    auto y_input_entity = atmo::core::ecs::EntityRegistry::Create<atmo::core::ecs::entities::UINumberInput>("Entity::UI::UIInput::UINumberInput");
    auto &input_entity_comp_y = y_input_entity->getComponentMutable<atmo::core::components::UIInput>();
    auto &y_input_entity_comp = y_input_entity->getComponentMutable<atmo::core::components::UINumberInput>();

    input_entity_comp_x.input_type = atmo::core::components::UIInput::InputType::Float;
    input_entity_comp_y.input_type = atmo::core::components::UIInput::InputType::Float;

    x_input_entity->setParent(*vector_container);
    y_input_entity->setParent(*vector_container);
    if (value) {
        atmo::core::types::Vector2 vector;

        field.get(value, &vector);

        spdlog::info("BEFORE : value: {}, prev value: {}", std::get<float>(x_input_entity_comp.value), std::get<float>(x_input_entity_comp.prev_value));
        x_input_entity_comp.value = vector.x;
        y_input_entity_comp.value = vector.y;
        spdlog::info("AFTER : value: {}, prev value: {}", std::get<float>(x_input_entity_comp.value), std::get<float>(x_input_entity_comp.prev_value));
    }

    return *vector_container;
}

ATMO_REGISTER_WIDGET("vector2", createVector2Widget, {});
