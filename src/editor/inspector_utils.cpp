#include "inspector_utils.hpp"

#include "core/ecs/entities/ui/ui.hpp"
#include "core/ecs/entities/ui/ui_label/ui_label.hpp"
#include "core/ecs/entity_registry.hpp"
#include "core/types.hpp"
#include "meta/widget_registry.hpp"

namespace atmo::editor
{
    std::vector<FieldWidgetRow> buildFieldWidgetRows(const meta::TypeInfo *ti, void *data, core::ecs::entities::Entity container)
    {
        std::vector<FieldWidgetRow> rows;
        if (!ti) {
            return rows;
        }

        for (auto &field_info : ti->fields) {
            auto input_container = core::ecs::EntityRegistry::Create<core::ecs::entities::UI>("Entity::UI");
            auto field_label = core::ecs::EntityRegistry::Create<core::ecs::entities::UILabel>("Entity::UI::UILabel");
            auto &input_container_layout = input_container->getComponentMutable<core::components::Layout>();

            input_container_layout.direction = core::components::Layout::Direction::Vertical;
            field_label->setFontSize(12);
            field_label->setText(field_info.name);
            field_label->getComponentMutable<core::components::UI>().modulate = core::types::Color::BLACK;
            field_label->setParent(*input_container);

            auto widget = meta::WidgetRegistry::Instance().create(*input_container, data, field_info);
            if (widget) {
                rows.push_back(FieldWidgetRow{ *widget, field_info });
            }

            input_container->setParent(container);
        }

        return rows;
    }
} // namespace atmo::editor
