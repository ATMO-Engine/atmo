#include "core/ecs/entities/entity.hpp"
#include "core/ecs/entities/ui/ui_button/ui_button.hpp"
#include "core/ecs/entities/ui/ui_checkbox/ui_checkbox.hpp"
#include "core/ecs/entities/ui/ui_layout.hpp"
#include "core/ecs/entity_registry.hpp"
#include "core/types.hpp"
#include "meta/widget_registry.hpp"

std::optional<atmo::core::ecs::entities::Entity> createBoolWidget(atmo::core::ecs::entities::Entity parent, void *value, const atmo::meta::FieldInfo &field)
{
    auto checkbox_entity = atmo::core::ecs::EntityRegistry::Create<atmo::core::ecs::entities::UICheckBox>("Entity::UI::UIRect::UIButton::UICheckBox");
    auto &checkbox_entity_layout = checkbox_entity->getComponentMutable<atmo::core::components::Layout>();

    checkbox_entity_layout.height.type = atmo::core::components::Layout::SizingAxis::SizingAxisType::FIXED;
    checkbox_entity_layout.height.size = atmo::core::components::Layout::SizingAxis::MinMax{ 32.0f, 32.0f };
    checkbox_entity_layout.aspect_ratio = { 1.0f, 1.0f };

    bool checked = false;
    if (value)
        field.get(value, &checked);

    checkbox_entity->getSignal<bool>("Toggle").connect([value, field](bool new_value) { field.set(value, &new_value); });

    return *checkbox_entity;
}

void updateBoolWidget(atmo::core::ecs::entities::Entity widget, void *value, const atmo::meta::FieldInfo &field)
{
    bool checked = false;
    field.get(value, &checked);
}

ATMO_REGISTER_WIDGET("bool", createBoolWidget, {}, updateBoolWidget);
