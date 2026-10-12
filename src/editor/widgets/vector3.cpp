#include "core/ecs/entities/entity.hpp"
#include "core/ecs/entities/ui/ui.hpp"
#include "core/ecs/entities/ui/ui_input/ui_input.hpp"
#include "core/ecs/entities/ui/ui_input/ui_number_input/ui_number_input.hpp"
#include "core/ecs/entity_registry.hpp"
#include "core/types.hpp"
#include "meta/widget_registry.hpp"

std::optional<atmo::core::ecs::entities::Entity> createVector3Widget(atmo::core::ecs::entities::Entity parent, void *value, const atmo::meta::FieldInfo &field)
{
    auto vector_container = atmo::core::ecs::EntityRegistry::Create<atmo::core::ecs::entities::UI>("Entity::UI");

    auto x_input_entity = atmo::core::ecs::EntityRegistry::Create<atmo::core::ecs::entities::UINumberInput>("Entity::UI::UIInput::UINumberInput");
    auto y_input_entity = atmo::core::ecs::EntityRegistry::Create<atmo::core::ecs::entities::UINumberInput>("Entity::UI::UIInput::UINumberInput");
    auto z_input_entity = atmo::core::ecs::EntityRegistry::Create<atmo::core::ecs::entities::UINumberInput>("Entity::UI::UIInput::UINumberInput");

    x_input_entity->getComponentMutable<atmo::core::components::UIInput>().input_type = atmo::core::components::UIInput::InputType::Float;
    y_input_entity->getComponentMutable<atmo::core::components::UIInput>().input_type = atmo::core::components::UIInput::InputType::Float;
    z_input_entity->getComponentMutable<atmo::core::components::UIInput>().input_type = atmo::core::components::UIInput::InputType::Float;

    if (value) {
        atmo::core::types::Vector3 vec;
        field.get(value, &vec);
        x_input_entity->getComponentMutable<atmo::core::components::UINumberInput>().value = vec.x;
        y_input_entity->getComponentMutable<atmo::core::components::UINumberInput>().value = vec.y;
        z_input_entity->getComponentMutable<atmo::core::components::UINumberInput>().value = vec.z;
    }

    x_input_entity->getSignal<float>("FloatValueChanged").connect([value, field](float val) {
        atmo::core::types::Vector3 vec;
        field.get(value, &vec);

        vec.x = val;
        field.set(value, &vec);
    });
    y_input_entity->getSignal<float>("FloatValueChanged").connect([value, field](float val) {
        atmo::core::types::Vector3 vec;
        field.get(value, &vec);

        vec.y = val;
        field.set(value, &vec);
    });
    z_input_entity->getSignal<float>("FloatValueChanged").connect([value, field](float val) {
        atmo::core::types::Vector3 vec;
        field.get(value, &vec);

        vec.z = val;
        field.set(value, &vec);
    });

    x_input_entity->setParent(*vector_container);
    y_input_entity->setParent(*vector_container);
    z_input_entity->setParent(*vector_container);

    return *vector_container;
}

void updateVector3Widget(atmo::core::ecs::entities::Entity widget, void *value, const atmo::meta::FieldInfo &field)
{
    auto children = widget.getChildren();
    if (children.size() < 3)
        return;
    atmo::core::types::Vector3 vec;
    field.get(value, &vec);
    children[0].getComponentMutable<atmo::core::components::UINumberInput>().value = vec.x;
    children[1].getComponentMutable<atmo::core::components::UINumberInput>().value = vec.y;
    children[2].getComponentMutable<atmo::core::components::UINumberInput>().value = vec.z;
}

ATMO_REGISTER_WIDGET("vector3", createVector3Widget, {}, updateVector3Widget);
