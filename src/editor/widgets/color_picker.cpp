#include "core/ecs/entities/entity.hpp"
#include "core/ecs/entity_registry.hpp"
#include "core/types.hpp"
#include "editor/editor_entities/ui_color_picker/ui_color_picker.hpp"
#include "meta/widget_registry.hpp"

std::optional<atmo::core::ecs::entities::Entity> createColorPickerWidget(
    atmo::core::ecs::entities::Entity parent, void *value, const atmo::meta::FieldInfo &field)
{
    auto picker_entity = atmo::core::ecs::EntityRegistry::Create<atmo::core::ecs::entities::UIColorPicker>("Entity::UI::UIRect::UIColorPicker");

    atmo::core::types::Color color = atmo::core::types::Color::WHITE;
    if (value)
        field.get(value, &color);
    picker_entity->setColor(color);

    picker_entity->getSignal<atmo::core::types::Color>("ColorChanged").connect([value, field](atmo::core::types::Color color) { field.set(value, &color); });

    return *picker_entity;
}

void updateColorPickerWidget(atmo::core::ecs::entities::Entity widget, void *value, const atmo::meta::FieldInfo &field)
{
    atmo::core::ecs::entities::UIColorPicker picker(widget.getHandle());

    atmo::core::types::Color color = atmo::core::types::Color::WHITE;
    field.get(value, &color);
    picker.setColor(color);
}

ATMO_REGISTER_WIDGET("color_picker", createColorPickerWidget, {}, updateColorPickerWidget);
