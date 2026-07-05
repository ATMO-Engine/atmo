#include "core/ecs/entities/entity.hpp"
#include "core/ecs/entities/ui/ui_checkbox/ui_checkbox.hpp"
#include "core/ecs/entities/ui/ui_layout.hpp"
#include "core/ecs/entity_registry.hpp"
#include "core/types.hpp"
#include "meta/widget_registry.hpp"

namespace
{
    void syncCheckBoxDisplay(atmo::core::ecs::entities::Entity widget, bool checked)
    {
        auto &comp = widget.getComponentMutable<atmo::core::components::UICheckBox>();
        comp.trigger = checked;

        auto &rect = widget.getComponentMutable<atmo::core::components::UIRect>();
        rect.color = checked ? atmo::core::types::Color::WHITE : atmo::core::types::Color::BLACK;
    }
} // namespace

std::optional<atmo::core::ecs::entities::Entity> createBoolWidget(atmo::core::ecs::entities::Entity parent, void *value, const atmo::meta::FieldInfo &field)
{
    auto checkbox_entity = atmo::core::ecs::EntityRegistry::Create<atmo::core::ecs::entities::UICheckBox>("Entity::UI::UIRect::UICheckBox");
    auto &checkbox_entity_layout = checkbox_entity->getComponentMutable<atmo::core::components::Layout>();

    checkbox_entity_layout.height.type = atmo::core::components::Layout::SizingAxis::SizingAxisType::FIXED;
    checkbox_entity_layout.height.size = atmo::core::components::Layout::SizingAxis::MinMax{ 32.0f, 32.0f };
    checkbox_entity_layout.aspect_ratio = { 1.0f, 1.0f };

    bool checked = false;
    if (value)
        field.get(value, &checked);
    syncCheckBoxDisplay(*checkbox_entity, checked);

    checkbox_entity->getSignal<atmo::core::ecs::entities::UICheckBox &>("Clicked").connect([value, field](atmo::core::ecs::entities::UICheckBox &chbox) {
        bool new_value = chbox.getComponentMutable<atmo::core::components::UICheckBox>().trigger;
        field.set(value, &new_value);
    });

    return *checkbox_entity;
}

void updateBoolWidget(atmo::core::ecs::entities::Entity widget, void *value, const atmo::meta::FieldInfo &field)
{
    bool checked = false;
    field.get(value, &checked);
    syncCheckBoxDisplay(widget, checked);
}

ATMO_REGISTER_WIDGET("bool", createBoolWidget, {}, updateBoolWidget);
