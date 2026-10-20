#include "ui_dropdown.hpp"
#include "clay.h"
#include "core/ecs/entities/ui/ui.hpp"
#include "core/ecs/entities/ui/ui_button/ui_button.hpp"
#include "core/ecs/entities/ui/ui_label/ui_label.hpp"
#include "core/ecs/entities/ui/ui_layout.hpp"
#include "core/ecs/entities/ui/ui_rect/ui_rect.hpp"
#include "core/ecs/entity_registry.hpp"
#include "core/event/event_registry.hpp"
#include "core/event/events/ui_event/dropdown_group_event/dropdown_group_event.hpp"
#include "core/types.hpp"
#include "meta/auto_register.hpp"

namespace atmo::core::ecs::entities
{
    void UIDropdown::RegisterSystems(flecs::world *world)
    {
        world->system("UIDropdown_ResetDrawDepth").kind(flecs::PreUpdate).run([](flecs::iter &it) {
            s_draw_depth.clear();
        });

        world->observer<components::UIDropdown>("UIDropdown_remove").event(flecs::OnRemove).each([](flecs::entity, components::UIDropdown &dropdown) {
            if (dropdown.group_event_listener_id != 0)
                event::EventRegistry::RemoveCallBack<event::events::DropdownGroupEvent>(dropdown.group_event_listener_id);
        });
    }

    void UIDropdown::initialize()
    {
        UIButton::initialize();

        setComponent<components::UIDropdown>({});

        auto handle = p_handle;
        auto eventId = event::EventRegistry::SetCallBack<event::events::DropdownGroupEvent>([handle](event::events::DropdownGroupEvent *evt) {
            auto dropDownEntity = UIDropdown(handle);
            if (!dropDownEntity.isAlive())
                return;
            auto &dropdown = dropDownEntity.getComponentMutable<components::UIDropdown>();
            if (dropdown.group == 0 || dropdown.group != evt->group_id || static_cast<flecs::entity_t>(dropDownEntity.getID()) == evt->sender_id)
                return;

            if (evt->action == event::events::DropdownGroupEvent::Action::Hovered) {
                if (dropdown.depth < evt->sender_depth)
                    dropdown.group_hovered = true;
                return;
            }

            if (!dropdown.is_open || dropdown.depth <= evt->sender_depth)
                return;
            dropdown.group_hovered = false;
            dropdown.is_open = false;
            dropDownEntity.getContent().getComponentMutable<components::UI>().visible = false;
            dropDownEntity.getSignal<>("Closed").emit();
        });
        getComponentMutable<components::UIDropdown>().group_event_listener_id = eventId;

        auto label = core::ecs::EntityRegistry::Create<core::ecs::entities::UILabel>("Entity::UI::UILabel");
        label->getComponentMutable<core::components::UI>().modulate = core::types::Color::BLACK;
        label->setFontSize(16);
        label->setParent(*this);

        auto content = core::ecs::EntityRegistry::Create<core::ecs::entities::UIRect>("Entity::UI::UIRect");
        auto &content_layout = content->getComponentMutable<core::components::Layout>();
        auto &content_rect = content->getComponentMutable<core::components::UIRect>();
        content_layout.direction = core::components::Layout::Direction::Vertical;
        content_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::FIT;
        content_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::FIT;
        content_layout.padding = { 4, 4, 4, 4 };
        content_layout.child_gap = 4;
        content_rect.corner_radius = { 4, 4, 4, 4 };
        content_layout.floating.enabled = true;
        content_layout.z_index = 100;
        content->setParent(*this);

        createSignal<>("Opened");
        createSignal<>("Closed");
    }

    UILabel UIDropdown::getLabel() const
    {
        return UILabel(getChildren()[0]);
    }

    UIRect UIDropdown::getContent() const
    {
        return UIRect(getChildren()[1]);
    }

    void UIDropdown::setText(const std::string &text)
    {
        getLabel().setText(text);
    }

    Clay_ElementDeclaration UIDropdown::buildDecl()
    {
        Clay_ElementDeclaration d = UIButton::buildDecl();

        return d;
    }

    bool UIDropdown::isHovered()
    {
        auto content = getContent();
        bool was_open = getComponentMutable<components::UIDropdown>().is_open;

        if (Clay_PointerOver(getComponent<components::UI>().element_id))
            return true;

        if (!was_open)
            return false;

        return Clay_PointerOver(content.getComponent<components::UI>().element_id);
    }

    void UIDropdown::dispatchGroupEvent(event::events::DropdownGroupEvent::Action action)
    {
        int group = getComponent<components::UIDropdown>().group;
        if (group == 0)
            return;

        auto evt = std::make_shared<event::events::DropdownGroupEvent>();
        evt->action = action;
        evt->group_id = group;
        evt->sender_id = static_cast<flecs::entity_t>(getID());
        evt->sender_depth = getComponent<components::UIDropdown>().depth;
        event::EventRegistry::Dispatch(evt);
    }

    void UIDropdown::draw(ClaySdL3RendererData *data)
    {
        UIButton::draw(data);

        auto label = getLabel();
        label.getComponentMutable<components::UI>().visible = !label.getText().empty();

        auto &dropdown = getComponentMutable<components::UIDropdown>();
        dropdown.depth = s_draw_depth[dropdown.group];
        bool is_hovered = isHovered();

        bool is_open = is_hovered || (dropdown.is_open && dropdown.group_hovered);
        dropdown.group_hovered = false;

        if (dropdown.is_open != is_open) {
            dropdown.is_open = is_open;
            if (is_open) {
                getSignal<>("Opened").emit();
            } else {
                getSignal<>("Closed").emit();
                dispatchGroupEvent(event::events::DropdownGroupEvent::Action::Closed);
            }
        }

        if (is_hovered)
            dispatchGroupEvent(event::events::DropdownGroupEvent::Action::Hovered);

        auto content = getContent();
        content.getComponentMutable<components::UI>().visible = dropdown.is_open;

        if (!dropdown.is_open)
            return;
        s_draw_depth[dropdown.group]++;
        spdlog::info("depth {}", s_draw_depth[dropdown.group]);

        Clay_ElementData data_self = Clay_GetElementData(getComponent<components::UI>().element_id);
        if (data_self.found)
            content.getComponentMutable<components::Layout>().floating.offest = {
                data_self.boundingBox.x, data_self.boundingBox.y + data_self.boundingBox.height
            };
    }
} // namespace atmo::core::ecs::entities

ATMO_REGISTER_ENTITY(entities::UIDropdown)
ATMO_REGISTER_COMPONENT(atmo::core::components::UIDropdown)
