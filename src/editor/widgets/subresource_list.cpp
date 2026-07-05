#include <cstddef>
#include <string>

#include "core/ecs/entities/entity.hpp"
#include "core/ecs/entities/ui/ui.hpp"
#include "core/ecs/entities/ui/ui_button/ui_button.hpp"
#include "core/ecs/entities/ui/ui_label/ui_label.hpp"
#include "core/ecs/entities/ui/ui_rect/ui_rect.hpp"
#include "core/ecs/entity_registry.hpp"
#include "core/resource/subresource_registry.hpp"
#include "core/types.hpp"
#include "editor/editor_entities/ui_popup/ui_popup.hpp"
#include "editor/inspector_utils.hpp"
#include "meta/meta_registry.hpp"
#include "meta/widget_registry.hpp"

namespace
{
    using atmo::core::ecs::entities::Entity;

    std::string shortName(std::string_view full_name)
    {
        auto pos = full_name.find_last_of("::");
        return std::string(pos == std::string_view::npos ? full_name : full_name.substr(pos + 1));
    }

    std::size_t currentSize(void *vec_ptr, const atmo::meta::FieldInfo &field)
    {
        return field.vector_size ? field.vector_size(vec_ptr) : 0;
    }

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

    void populateSubresourceListWidget(Entity widget_root, void *value, const atmo::meta::FieldInfo &field);

    void openAddShapePopup(Entity widget_root, void *value, const atmo::meta::FieldInfo &field)
    {
        auto window = atmo::core::ecs::entities::UI(widget_root.getHandle()).getWindow();
        if (!window || window->getChildren().empty())
            return;
        auto popup_parent = window->getChildren()[0];

        auto popup = atmo::core::ecs::EntityRegistry::Create<atmo::core::ecs::entities::UIPopup>("Entity::UI::UIRect::UIPopup");

        auto bg = atmo::core::ecs::EntityRegistry::Create<atmo::core::ecs::entities::UIRect>("Entity::UI::UIRect");
        auto &bg_layout = bg->getComponentMutable<atmo::core::components::Layout>();
        bg_layout.width.type = atmo::core::components::Layout::SizingAxis::SizingAxisType::PERCENT;
        bg_layout.width.size = 0.3f;
        bg_layout.height.type = atmo::core::components::Layout::SizingAxis::SizingAxisType::PERCENT;
        bg_layout.height.size = 0.5f;
        bg_layout.direction = atmo::core::components::Layout::Direction::Vertical;
        bg_layout.padding = { 8, 8, 8, 8 };
        bg_layout.child_gap = 8;
        bg->getComponentMutable<atmo::core::components::UIRect>().color = atmo::core::types::Color("#9f9f9f");
        bg->setParent(*popup);
        popup->setParent(popup_parent);

        auto top_bar = atmo::core::ecs::EntityRegistry::Create<atmo::core::ecs::entities::UI>("Entity::UI");
        auto &top_bar_layout = top_bar->getComponentMutable<atmo::core::components::Layout>();
        top_bar_layout.direction = atmo::core::components::Layout::Direction::Horizontal;
        top_bar_layout.width.type = atmo::core::components::Layout::SizingAxis::SizingAxisType::GROW;
        top_bar_layout.child_alignment.vertical = atmo::core::components::Layout::ChildAlignment::Center;
        top_bar->setParent(*bg);

        auto title = atmo::core::ecs::EntityRegistry::Create<atmo::core::ecs::entities::UILabel>("Entity::UI::UILabel");
        title->setText("atmo.add_shape");
        title->setFontSize(16);
        title->getComponentMutable<atmo::core::components::Layout>().width.type = atmo::core::components::Layout::SizingAxis::SizingAxisType::GROW;
        title->setParent(*top_bar);

        auto close_button = makeSmallButton("x");
        close_button.getSignal<>("Released").connect([popup]() { popup->destroy(); });
        close_button.setParent(*top_bar);

        auto list = atmo::core::ecs::EntityRegistry::Create<atmo::core::ecs::entities::UI>("Entity::UI");
        auto &list_layout = list->getComponentMutable<atmo::core::components::Layout>();
        list_layout.direction = atmo::core::components::Layout::Direction::Vertical;
        list_layout.width.type = atmo::core::components::Layout::SizingAxis::SizingAxisType::GROW;
        list_layout.child_gap = 4;
        list->setParent(*bg);

        const std::string prefix = field.subresource_base_full_name ? std::string(field.subresource_base_full_name) + "::" : "";
        for (auto &name : atmo::core::resource::SubResourceRegistry::GetEntries()) {
            if (prefix.empty() || !name.starts_with(prefix) || atmo::core::resource::SubResourceRegistry::IsAbstract(name))
                continue;

            auto entry_button = makeSmallButton(shortName(name));
            entry_button.getComponentMutable<atmo::core::components::Layout>().width.type = atmo::core::components::Layout::SizingAxis::SizingAxisType::GROW;
            entry_button.getSignal<>("Released").connect([widget_root, value, field, name, popup]() {
                if (field.vector_push_subresource)
                    field.vector_push_subresource(value, name);
                popup->destroy();
                populateSubresourceListWidget(widget_root, value, field);
            });
            entry_button.setParent(*list);
        }
    }

    void populateSubresourceListWidget(Entity widget_root, void *value, const atmo::meta::FieldInfo &field)
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
        add_button.getSignal<>("Released").connect([widget_root, value, field]() { openAddShapePopup(widget_root, value, field); });
        add_button.setParent(*header);

        auto clear_button = makeSmallButton("Clear");
        clear_button.getSignal<>("Released").connect([widget_root, value, field]() {
            if (field.vector_clear)
                field.vector_clear(value);
            populateSubresourceListWidget(widget_root, value, field);
        });
        clear_button.setParent(*header);

        header->setParent(widget_root);

        auto rows = atmo::core::ecs::EntityRegistry::Create<atmo::core::ecs::entities::UI>("Entity::UI");
        auto &rows_layout = rows->getComponentMutable<atmo::core::components::Layout>();
        rows_layout.direction = atmo::core::components::Layout::Direction::Vertical;
        rows_layout.width.type = atmo::core::components::Layout::SizingAxis::SizingAxisType::GROW;
        rows_layout.child_gap = 8;

        const std::size_t count = currentSize(value, field);
        for (std::size_t i = 0; i < count; ++i) {
            void *elem_ptr = field.vector_element_ptr ? field.vector_element_ptr(value, i) : nullptr;
            if (!elem_ptr)
                continue;

            auto row_group = atmo::core::ecs::EntityRegistry::Create<atmo::core::ecs::entities::UIRect>("Entity::UI::UIRect");
            auto &row_group_layout = row_group->getComponentMutable<atmo::core::components::Layout>();
            row_group_layout.direction = atmo::core::components::Layout::Direction::Vertical;
            row_group_layout.width.type = atmo::core::components::Layout::SizingAxis::SizingAxisType::GROW;
            row_group_layout.padding = { 4, 4, 4, 4 };
            row_group_layout.child_gap = 4;
            row_group->getComponentMutable<atmo::core::components::UIRect>().color.a = 0.08f;

            auto row_header = atmo::core::ecs::EntityRegistry::Create<atmo::core::ecs::entities::UI>("Entity::UI");
            auto &row_header_layout = row_header->getComponentMutable<atmo::core::components::Layout>();
            row_header_layout.direction = atmo::core::components::Layout::Direction::Horizontal;
            row_header_layout.width.type = atmo::core::components::Layout::SizingAxis::SizingAxisType::GROW;
            row_header_layout.child_alignment.vertical = atmo::core::components::Layout::ChildAlignment::Center;

            std::string type_name = "Unknown";
            if (field.subresource_element_type_name)
                type_name = shortName(field.subresource_element_type_name(elem_ptr));

            auto type_label = atmo::core::ecs::EntityRegistry::Create<atmo::core::ecs::entities::UILabel>("Entity::UI::UILabel");
            type_label->setFontSize(12);
            type_label->setText(type_name);
            type_label->getComponentMutable<atmo::core::components::Layout>().width.type = atmo::core::components::Layout::SizingAxis::SizingAxisType::GROW;
            type_label->setParent(*row_header);

            auto remove_button = makeSmallButton("x");
            remove_button.getSignal<>("Released").connect([widget_root, value, field, i]() {
                if (field.vector_erase)
                    field.vector_erase(value, i);
                populateSubresourceListWidget(widget_root, value, field);
            });
            remove_button.setParent(*row_header);

            row_header->setParent(*row_group);

            if (field.subresource_element_type_name) {
                const auto *elem_ti = atmo::meta::MetaRegistry::Instance().find(field.subresource_element_type_name(elem_ptr));
                atmo::editor::buildFieldWidgetRows(elem_ti, elem_ptr, *row_group);
            }

            row_group->setParent(*rows);
        }

        rows->setParent(widget_root);
    }
} // namespace

std::optional<atmo::core::ecs::entities::Entity>
createSubresourceListWidget(atmo::core::ecs::entities::Entity parent, void *value, const atmo::meta::FieldInfo &field)
{
    auto widget_root = atmo::core::ecs::EntityRegistry::Create<atmo::core::ecs::entities::UI>("Entity::UI");
    auto &layout = widget_root->getComponentMutable<atmo::core::components::Layout>();
    layout.direction = atmo::core::components::Layout::Direction::Vertical;
    layout.width.type = atmo::core::components::Layout::SizingAxis::SizingAxisType::GROW;
    layout.child_gap = 4;

    populateSubresourceListWidget(*widget_root, resolveVectorPtr(value, field), field);

    return *widget_root;
}

void updateSubresourceListWidget(atmo::core::ecs::entities::Entity widget, void *value, const atmo::meta::FieldInfo &field)
{
    auto *vec_ptr = resolveVectorPtr(value, field);
    auto children = widget.getChildren();
    const std::size_t rendered_count = children.size() >= 2 ? children[1].getChildren().size() : 0;

    if (rendered_count != currentSize(vec_ptr, field)) {
        populateSubresourceListWidget(widget, vec_ptr, field);
    }
}

ATMO_REGISTER_WIDGET("subresource_list", createSubresourceListWidget, {}, updateSubresourceListWidget);
