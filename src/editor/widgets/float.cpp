#include "core/ecs/entities/entity.hpp"
#include "core/ecs/entities/ui/ui_input/ui_input.hpp"
#include "core/ecs/entities/ui/ui_input/ui_number_input/ui_number_input.hpp"
#include "core/ecs/entities/ui/ui_label/ui_label.hpp"
#include "core/ecs/entity_registry.hpp"
#include "meta/widget_registry.hpp"

std::optional<atmo::core::ecs::entities::Entity> createFloatWidget(atmo::core::ecs::entities::Entity parent, void *value, const atmo::meta::FieldInfo &field)
{
    auto float_input_entity = atmo::core::ecs::EntityRegistry::Create<atmo::core::ecs::entities::UINumberInput>("Entity::UI::UIInput::UINumberInput");
    auto &input_entity_comp = float_input_entity->getComponentMutable<atmo::core::components::UIInput>();
    auto &float_input_entity_comp = float_input_entity->getComponentMutable<atmo::core::components::UINumberInput>();

    input_entity_comp.input_type = atmo::core::components::UIInput::InputType::Float;
    if (value)
        field.get(value, &float_input_entity_comp.value);

    float_input_entity->getSignal<float>("FloatValueChanged").connect([value, field](float val) { field.set(value, &val); });

    return *float_input_entity;
}

void updateFloatWidget(atmo::core::ecs::entities::Entity widget, void *value, const atmo::meta::FieldInfo &field)
{
    auto &comp = widget.getComponentMutable<atmo::core::components::UINumberInput>();
    field.get(value, &comp.value);
}

ATMO_REGISTER_WIDGET("float", createFloatWidget, {}, updateFloatWidget);
