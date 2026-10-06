#include "ui_list.hpp"
#include "core/ecs/entities/ui/ui_button/ui_button.hpp"
#include "core/ecs/entities/ui/ui_label/ui_label.hpp"
#include "core/ecs/entity_registry.hpp"
#include "core/types.hpp"
#include "meta/auto_register.hpp"

namespace atmo::core::ecs::entities
{
    void UIList::RegisterSystems(flecs::world *) {}

    void UIList::initialize()
    {
        UIRect::initialize();
        setComponent<components::UIList>({});

        createSignal<int>("ItemSelected");

        auto &lay = getComponentMutable<core::components::Layout>();
        lay.direction = core::components::Layout::Direction::Vertical;
        lay.child_gap = 4;
        lay.padding = { 4, 4, 4, 4 };
        lay.width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        lay.height.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        lay.clip.horizontal = true;
        lay.clip.vertical = true;

        getComponentMutable<core::components::UIRect>().color = core::types::Color::TRANSPARENT_COL;
    }

    void UIList::setDirection(core::components::Layout::Direction direction)
    {
        getComponentMutable<core::components::Layout>().direction = direction;

        for (auto &child : getChildren())
            applyItemLayout(child);
    }

    void UIList::setItems(const std::vector<std::string> &items)
    {
        auto &comp = getComponentMutable<components::UIList>();
        comp.items = items;
        if (comp.selected >= (int)comp.items.size())
            comp.selected = -1;

        rebuild();
    }

    void UIList::addItem(const std::string &item)
    {
        getComponentMutable<components::UIList>().items.push_back(item);
        rebuild();
    }

    void UIList::removeItem(int index)
    {
        auto &comp = getComponentMutable<components::UIList>();
        if (index < 0 || index >= (int)comp.items.size()) {
            spdlog::warn("UIList: item {} out of range, nothing removed", index);
            return;
        }

        comp.items.erase(comp.items.begin() + index);
        if (comp.selected == index)
            comp.selected = -1;
        else if (comp.selected > index)
            comp.selected--;

        rebuild();
    }

    void UIList::clear()
    {
        auto &comp = getComponentMutable<components::UIList>();
        comp.items.clear();
        comp.selected = -1;

        rebuild();
    }

    void UIList::select(int index, bool emit)
    {
        auto &comp = getComponentMutable<components::UIList>();
        if (index < -1 || index >= (int)comp.items.size()) {
            spdlog::warn("UIList: item {} out of range, selection unchanged", index);
            return;
        }

        comp.selected = index;

        auto children = getChildren();
        for (int i = 0; i < (int)children.size(); i++) {
            auto &rect = children[i].getComponentMutable<core::components::UIRect>();
            rect.color = i == index ? HighlightColor : core::types::Color::TRANSPARENT_COL;
        }

        if (emit)
            getSignal<int>("ItemSelected").emit(index);
    }

    int UIList::getSelected() const
    {
        return getComponent<components::UIList>().selected;
    }

    std::string UIList::getSelectedItem() const
    {
        const auto &comp = getComponent<components::UIList>();
        if (comp.selected < 0 || comp.selected >= (int)comp.items.size())
            return {};

        return comp.items[comp.selected];
    }

    void UIList::applyItemLayout(Entity &item) const
    {
        const auto &comp = getComponent<components::UIList>();
        const bool horizontal = getComponent<core::components::Layout>().direction == core::components::Layout::Direction::Horizontal;
        auto &lay = item.getComponentMutable<core::components::Layout>();

        auto &main = horizontal ? lay.width : lay.height;
        auto &cross = horizontal ? lay.height : lay.width;

        main.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        cross.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
    }

    void UIList::rebuild()
    {
        for (auto &child : getChildren()) child.destroy();

        const auto &comp = getComponent<components::UIList>();
        auto handle = p_handle;

        for (int i = 0; i < (int)comp.items.size(); i++) {
            auto item = core::ecs::EntityRegistry::Create<UIButton>("Entity::UI::UIRect::UIButton");
            auto item_label = core::ecs::EntityRegistry::Create<UILabel>("Entity::UI::UILabel");
            auto &item_rect = item->getComponentMutable<core::components::UIRect>();

            item_rect.color = i == comp.selected ? HighlightColor : core::types::Color::TRANSPARENT_COL;
            item_rect.corner_radius = { 4, 4, 4, 4 };
            item->getComponentMutable<core::components::Layout>().child_alignment.horizontal = core::components::Layout::ChildAlignment::Start;
            item->getComponentMutable<core::components::Layout>().padding.left = 8;
            applyItemLayout(*item);

            item_label->setText(comp.items[i]);
            item_label->setFontSize(12);
            item_label->getComponentMutable<core::components::UI>().modulate = core::types::Color::BLACK;
            item_label->setParent(*item);
            item->setParent(*this);

            item->getSignal<>("Pressed").connect([handle, i]() {
                if (!handle.is_alive())
                    return;
                UIList list(core::ecs::EntityRegistry::GetEntityFromId(handle));
                list.select(i);
            });
        }
    }

    Clay_ElementDeclaration UIList::buildDecl()
    {
        return UIRect::buildDecl();
    }

    void UIList::draw(ClaySdL3RendererData *) {}
} // namespace atmo::core::ecs::entities

ATMO_REGISTER_ENTITY(entities::UIList)
ATMO_REGISTER_COMPONENT(atmo::core::components::UIList)
