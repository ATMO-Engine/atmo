#include "ui_dropdown_menu.hpp"
#include "clay.h"
#include "core/ecs/entities/ui/ui.hpp"
#include "core/ecs/entities/ui/ui_button/ui_button.hpp"
#include "core/ecs/entities/ui/ui_label/ui_label.hpp"
#include "core/ecs/entities/ui/ui_layout.hpp"
#include "core/ecs/entities/ui/ui_rect/ui_rect.hpp"
#include "core/ecs/entity_registry.hpp"
#include "core/event/event_registry.hpp"
#include "core/event/events/ui_event/dropdown_group_event/dropdown_group_event.hpp"
#include "core/input/input_manager.hpp"
#include "core/types.hpp"
#include "meta/auto_register.hpp"

namespace atmo::core::ecs::entities
{
    void UiDropdownMenu::RegisterSystems(flecs::world *world)
    {
        world->system("UiDropdownMenu_ResetDrawDepth").kind(flecs::PreUpdate).run([](flecs::iter &it) { s_draw_depth.clear(); });

        world->observer<components::UiDropdownMenu>("UiDropdownMenu_remove")
            .event(flecs::OnRemove)
            .each([](flecs::entity e, components::UiDropdownMenu &dropdown) {
                if (dropdown.group_event_listener_id != 0)
                    event::EventRegistry::RemoveCallBack<event::events::DropdownGroupEvent>(dropdown.group_event_listener_id);
                std::erase_if(s_open_child, [e](const auto &entry) { return entry.second == e.id(); });
            });
    }

    void UiDropdownMenu::initialize()
    {
        UIButton::initialize();

        setComponent<components::UiDropdownMenu>({});

        auto handle = p_handle;
        auto eventId = event::EventRegistry::SetCallBack<event::events::DropdownGroupEvent>([handle](event::events::DropdownGroupEvent *evt) {
            auto dropDownEntity = UiDropdownMenu(handle);
            if (!dropDownEntity.isAlive())
                return;
            auto &dropdown = dropDownEntity.getComponentMutable<components::UiDropdownMenu>();
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
        getComponentMutable<components::UiDropdownMenu>().group_event_listener_id = eventId;

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

    UILabel UiDropdownMenu::getLabel() const
    {
        return UILabel(getChildren()[0]);
    }

    UIRect UiDropdownMenu::getContent() const
    {
        return UIRect(getChildren()[1]);
    }

    void UiDropdownMenu::setText(const std::string &text)
    {
        getLabel().setText(text);
    }

    Clay_ElementDeclaration UiDropdownMenu::buildDecl()
    {
        Clay_ElementDeclaration d = UIButton::buildDecl();

        return d;
    }

    bool UiDropdownMenu::isHovered()
    {
        auto content = getContent();
        bool was_open = getComponentMutable<components::UiDropdownMenu>().is_open;

        if (Clay_PointerOver(getComponent<components::UI>().element_id))
            return true;

        if (!was_open)
            return false;

        return Clay_PointerOver(content.getComponent<components::UI>().element_id);
    }

    bool UiDropdownMenu::isHeadingToContent(const types::Vector2 &from, const types::Vector2 &move) const
    {
        Clay_ElementData data = Clay_GetElementData(getContent().getComponent<components::UI>().element_id);
        if (!data.found)
            return false;

        const auto &box = data.boundingBox;
        const float left = box.x;
        const float right = box.x + box.width;
        const float top = box.y;
        const float bottom = box.y + box.height;

        const bool beside = from.x < left || from.x > right;
        const float edge_x = from.x < left ? left : right;
        const float edge_y = from.y < top ? top : bottom;
        const types::Vector2 corner_a = beside ? types::Vector2{ edge_x, top } : types::Vector2{ left, edge_y };
        const types::Vector2 corner_b = beside ? types::Vector2{ edge_x, bottom } : types::Vector2{ right, edge_y };

        const types::Vector2 to_a = { corner_a.x - from.x, corner_a.y - from.y };
        const types::Vector2 to_b = { corner_b.x - from.x, corner_b.y - from.y };


        const auto cross = [](const types::Vector2 &u, const types::Vector2 &v) { return u.x * v.y - u.y * v.x; };

        const float side = cross(to_a, to_b);
        return cross(to_a, move) * side >= 0.0f && cross(move, to_b) * side >= 0.0f;
    }

    bool UiDropdownMenu::updateAim(float delta_time)
    {
        auto &dropdown = getComponentMutable<components::UiDropdownMenu>();
        const auto mouse = core::InputManager::GetMousePosition();

        if (!dropdown.is_open || isHovered()) {
            dropdown.aim_sample = mouse;
            dropdown.aim_idle_time = 0.0f;
            return false;
        }

        if (!dropdown.aim_idle_time)
            return false;

        *dropdown.aim_idle_time += delta_time;

        const types::Vector2 move = { mouse.x - dropdown.aim_sample.x, mouse.y - dropdown.aim_sample.y };
        if (move.x * move.x + move.y * move.y < AIM_MIN_MOVE * AIM_MIN_MOVE) {
            if (*dropdown.aim_idle_time <= AIM_TIMEOUT)
                return true;
            dropdown.aim_idle_time.reset();
            return false;
        }

        if (!isHeadingToContent(dropdown.aim_sample, move)) {
            dropdown.aim_idle_time.reset();
            return false;
        }

        dropdown.aim_sample = mouse;
        dropdown.aim_idle_time = 0.0f;
        return true;
    }

    bool UiDropdownMenu::isBlockedBySibling()
    {
        auto it = s_open_child.find(p_handle.parent().id());
        if (it == s_open_child.end() || it->second == static_cast<flecs::entity_t>(getID()))
            return false;

        return UiDropdownMenu(core::ecs::EntityRegistry::GetEntityFromId(it->second)).updateAim(0.0f);
    }

    void UiDropdownMenu::dispatchGroupEvent(event::events::DropdownGroupEvent::Action action)
    {
        int group = getComponent<components::UiDropdownMenu>().group;
        if (group == 0)
            return;

        auto evt = std::make_shared<event::events::DropdownGroupEvent>();
        evt->action = action;
        evt->group_id = group;
        evt->sender_id = static_cast<flecs::entity_t>(getID());
        evt->sender_depth = getComponent<components::UiDropdownMenu>().depth;
        event::EventRegistry::Dispatch(evt);
    }

    void UiDropdownMenu::draw(float delta_time)
    {
        const bool blocked = isBlockedBySibling();
        if (!blocked)
            UIButton::draw(delta_time);

        auto label = getLabel();
        label.getComponentMutable<components::UI>().visible = !label.getText().empty();

        auto &dropdown = getComponentMutable<components::UiDropdownMenu>();
        dropdown.depth = s_draw_depth[dropdown.group];
        bool is_hovered = !blocked && isHovered();
        bool aiming = updateAim(delta_time);

        bool is_open = is_hovered || aiming || (dropdown.is_open && dropdown.group_hovered);
        dropdown.group_hovered = false;

        if (dropdown.is_open != is_open) {
            dropdown.is_open = is_open;
            if (is_open) {
                s_open_child[p_handle.parent().id()] = static_cast<flecs::entity_t>(getID());
                getSignal<>("Opened").emit();
            } else {
                getSignal<>("Closed").emit();
                dispatchGroupEvent(event::events::DropdownGroupEvent::Action::Closed);
            }
        }

        if (is_hovered || aiming)
            dispatchGroupEvent(event::events::DropdownGroupEvent::Action::Hovered);

        auto content = getContent();
        content.getComponentMutable<components::UI>().visible = dropdown.is_open;

        if (!dropdown.is_open)
            return;
        s_draw_depth[dropdown.group]++;

        Clay_ElementData data_self = Clay_GetElementData(getComponent<components::UI>().element_id);
        if (data_self.found) {
            const auto &box = data_self.boundingBox;
            types::Vector2 offset = { box.x, box.y + box.height };
            if (dropdown.open_side == components::UiDropdownMenu::OpenSide::RIGHT)
                offset = { box.x + box.width, box.y };
            content.getComponentMutable<components::Layout>().floating.offest = offset;
        }
    }
} // namespace atmo::core::ecs::entities

ATMO_REGISTER_ENTITY(entities::UiDropdownMenu)
ATMO_REGISTER_COMPONENT(atmo::core::components::UiDropdownMenu)
