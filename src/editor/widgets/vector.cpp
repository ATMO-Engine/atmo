#include <cstddef>
#include <string>

#include "core/ecs/entities/entity.hpp"
#include "core/ecs/entities/ui/ui.hpp"
#include "core/ecs/entities/ui/ui_button/ui_button.hpp"
#include "core/ecs/entities/ui/ui_label/ui_label.hpp"
#include "core/ecs/entity_registry.hpp"
#include "core/types.hpp"
#include "meta/widget_registry.hpp"

namespace
{
    using atmo::core::ecs::entities::Entity;

    std::size_t currentSize(void *vec_ptr, const atmo::meta::FieldInfo &field)
    {
        return field.vector_size ? field.vector_size(vec_ptr) : 0;
    }

    // WidgetRegistry always hands widgets the owning component/struct pointer (matching every other widget's
    // field.get/field.set-based convention) rather than a direct pointer to the field itself. The vector_* function
    // pointers on FieldInfo operate directly on the vector's own memory, so resolve that address once via the
    // field's byte offset before touching them.
    void *resolveVectorPtr(void *owner, const atmo::meta::FieldInfo &field)
    {
        return static_cast<char *>(owner) + field.offset;
    }

    Entity makeSmallButton(const std::string &text)
    {
        auto button = atmo::core::ecs::EntityRegistry::Create<atmo::core::ecs::entities::UIButton>("Entity::UI::UIRect::UIButton");
        button->getChildren()[0].destroy();

        auto &layout = button->getComponentMutable<atmo::core::components::Layout>();
        layout.width.type = atmo::core::components::Layout::SizingAxis::SizingAxisType::FIXED;
        layout.width.size = atmo::core::components::Layout::SizingAxis::MinMax{ 22.0f, 22.0f };
        layout.height.type = atmo::core::components::Layout::SizingAxis::SizingAxisType::FIXED;
        layout.height.size = atmo::core::components::Layout::SizingAxis::MinMax{ 22.0f, 22.0f };

        auto label = atmo::core::ecs::EntityRegistry::Create<atmo::core::ecs::entities::UILabel>("Entity::UI::UILabel");
        label->setFontSize(12);
        label->setText(text);
        label->setTextAlignment(atmo::core::components::UILabel::TextAlignment::ALIGN_CENTER);
        label->setParent(*button);

        return *button;
    }

    void populateVectorWidget(Entity widget_root, void *value, const atmo::meta::FieldInfo &field)
    {
        for (auto &child : widget_root.getChildren()) child.destroy();

        auto header = atmo::core::ecs::EntityRegistry::Create<atmo::core::ecs::entities::UI>("Entity::UI");
        auto &header_layout = header->getComponentMutable<atmo::core::components::Layout>();
        header_layout.direction = atmo::core::components::Layout::Direction::Horizontal;
        header_layout.width.type = atmo::core::components::Layout::SizingAxis::SizingAxisType::GROW;
        header_layout.child_gap = 4;
        header_layout.child_alignment.vertical = atmo::core::components::Layout::ChildAlignment::Center;

        auto count_label = atmo::core::ecs::EntityRegistry::Create<atmo::core::ecs::entities::UILabel>("Entity::UI::UILabel");
        count_label->setFontSize(11);
        count_label->setText(std::to_string(currentSize(value, field)) + " elements");
        count_label->getComponentMutable<atmo::core::components::UI>().modulate = atmo::core::types::Color::BLACK;
        count_label->setParent(*header);

        auto spacer = atmo::core::ecs::EntityRegistry::Create<atmo::core::ecs::entities::UI>("Entity::UI");
        spacer->getComponentMutable<atmo::core::components::Layout>().width.type = atmo::core::components::Layout::SizingAxis::SizingAxisType::GROW;
        spacer->setParent(*header);

        auto add_button = makeSmallButton("+");
        add_button.getSignal<>("Released").connect([widget_root, value, field]() {
            if (field.vector_push_default)
                field.vector_push_default(value);
            populateVectorWidget(widget_root, value, field);
        });
        add_button.setParent(*header);

        auto clear_button = makeSmallButton("Clear");
        clear_button.getSignal<>("Released").connect([widget_root, value, field]() {
            if (field.vector_clear)
                field.vector_clear(value);
            populateVectorWidget(widget_root, value, field);
        });
        clear_button.setParent(*header);

        header->setParent(widget_root);

        auto rows = atmo::core::ecs::EntityRegistry::Create<atmo::core::ecs::entities::UI>("Entity::UI");
        auto &rows_layout = rows->getComponentMutable<atmo::core::components::Layout>();
        rows_layout.direction = atmo::core::components::Layout::Direction::Vertical;
        rows_layout.width.type = atmo::core::components::Layout::SizingAxis::SizingAxisType::GROW;
        rows_layout.child_gap = 4;

        const std::size_t count = currentSize(value, field);
        for (std::size_t i = 0; i < count; ++i) {
            auto row = atmo::core::ecs::EntityRegistry::Create<atmo::core::ecs::entities::UI>("Entity::UI");
            auto &row_layout = row->getComponentMutable<atmo::core::components::Layout>();
            row_layout.direction = atmo::core::components::Layout::Direction::Horizontal;
            row_layout.width.type = atmo::core::components::Layout::SizingAxis::SizingAxisType::GROW;
            row_layout.child_gap = 4;
            row_layout.child_alignment.vertical = atmo::core::components::Layout::ChildAlignment::Center;

            auto elem_container = atmo::core::ecs::EntityRegistry::Create<atmo::core::ecs::entities::UI>("Entity::UI");
            elem_container->getComponentMutable<atmo::core::components::Layout>().width.type = atmo::core::components::Layout::SizingAxis::SizingAxisType::GROW;

            atmo::meta::FieldInfo row_field{};
            row_field.name = "element";
            row_field.widget = field.element_widget;
            row_field.get = field.element_get;
            row_field.set = field.element_set;
            row_field.size = field.element_size;

            void *elem_ptr = field.vector_element_ptr ? field.vector_element_ptr(value, i) : nullptr;
            auto elem_widget = atmo::meta::WidgetRegistry::Instance().create(*elem_container, elem_ptr, row_field);
            (void)elem_widget;
            elem_container->setParent(*row);

            auto remove_button = makeSmallButton("x");
            remove_button.getSignal<>("Released").connect([widget_root, value, field, i]() {
                if (field.vector_erase)
                    field.vector_erase(value, i);
                populateVectorWidget(widget_root, value, field);
            });
            remove_button.setParent(*row);

            row->setParent(*rows);
        }

        rows->setParent(widget_root);
    }
} // namespace

std::optional<atmo::core::ecs::entities::Entity>
createVectorWidget(atmo::core::ecs::entities::Entity parent, void *value, const atmo::meta::FieldInfo &field)
{
    auto widget_root = atmo::core::ecs::EntityRegistry::Create<atmo::core::ecs::entities::UI>("Entity::UI");
    auto &layout = widget_root->getComponentMutable<atmo::core::components::Layout>();
    layout.direction = atmo::core::components::Layout::Direction::Vertical;
    layout.width.type = atmo::core::components::Layout::SizingAxis::SizingAxisType::GROW;
    layout.child_gap = 4;

    populateVectorWidget(*widget_root, resolveVectorPtr(value, field), field);

    return *widget_root;
}

void updateVectorWidget(atmo::core::ecs::entities::Entity widget, void *value, const atmo::meta::FieldInfo &field)
{
    auto *vec_ptr = resolveVectorPtr(value, field);
    auto children = widget.getChildren();
    const std::size_t rendered_count = children.size() >= 2 ? children[1].getChildren().size() : 0;

    if (rendered_count != currentSize(vec_ptr, field)) {
        populateVectorWidget(widget, vec_ptr, field);
    }
}

ATMO_REGISTER_WIDGET("vector", createVectorWidget, {}, updateVectorWidget);
