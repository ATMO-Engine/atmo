#include "ui_foldable_tree_item.hpp"
#include "clay.h"
#include "core/ecs/entities/ui/ui.hpp"
#include "core/ecs/entities/ui/ui_button/ui_button.hpp"
#include "core/ecs/entities/ui/ui_checkbox/ui_checkbox.hpp"
#include "core/ecs/entities/ui/ui_layout.hpp"
#include "core/ecs/entities/ui/ui_rect/ui_rect.hpp"
#include "core/ecs/entity_registry.hpp"
#include "meta/auto_register.hpp"


namespace atmo::core::ecs::entities
{
    void UIFoldableTreeItem::RegisterSystems(flecs::world *world) {}

    void UIFoldableTreeItem::initialize()
    {
        UIButton::initialize();

        setComponent<components::UIFoldableTreeItem>({});

        auto title_bar = core::ecs::EntityRegistry::Create("Entity::UI::UIRect");
        auto &title_bar_layout = title_bar->getComponentMutable<core::components::Layout>();
        auto &title_bar_rect = title_bar->getComponentMutable<core::components::UIRect>();
        title_bar_rect.color.a = 0.0f;
        title_bar_layout.direction = components::Layout::Direction::Horizontal;
        title_bar_layout.child_gap = 8;
        title_bar_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::FIXED;
        title_bar_layout.height.size = core::components::Layout::SizingAxis::MinMax{ 24.0f, 24.0f };
        title_bar_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        title_bar_layout.child_alignment.horizontal = core::components::Layout::ChildAlignment::Start;
        title_bar_layout.child_alignment.vertical = core::components::Layout::ChildAlignment::Start;
        title_bar->setParent(*this);

        auto openbox = core::ecs::EntityRegistry::Create<core::ecs::entities::UICheckBox>("Entity::UI::UIRect::UICheckBox");
        auto &openbox_layout = openbox->getComponentMutable<core::components::Layout>();
        openbox_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::FIXED;
        openbox_layout.width.size = core::components::Layout::SizingAxis::MinMax(18.0f, 18.0f);
        openbox_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::FIXED;
        openbox_layout.height.size = core::components::Layout::SizingAxis::MinMax(18.0f, 18.0f);

        openbox->setParent(*title_bar);
        openbox->getSignal<core::ecs::entities::UICheckBox &>("Clicked").connect([](core::ecs::entities::UICheckBox &chBox) {
            auto &fodableTreeComp = chBox.getParent().getParent().getComponentMutable<core::components::UIFoldableTreeItem>();
            auto &chBoxComp = chBox.getComponentMutable<core::components::UICheckBox>();

            fodableTreeComp.open = chBoxComp.trigger;
        });

        getChildren()[0].setParent(*title_bar);

        auto child_container = core::ecs::EntityRegistry::Create<core::ecs::entities::UIRect>("Entity::UI::UIRect");
        auto &child_container_layout = child_container->getComponentMutable<core::components::Layout>();
        auto &child_container_rect = child_container->getComponentMutable<core::components::UIRect>();
        child_container_rect.color.a = 0.0f;
        child_container_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::FIT;
        child_container_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::FIT;
        child_container_layout.direction = core::components::Layout::Direction::Vertical;
        child_container_layout.child_gap = 8;
        child_container_layout.padding = { 16, 0, 12, 0 };
        child_container->setParent(*this);
    }

    Clay_ElementDeclaration UIFoldableTreeItem::buildDecl()
    {
        Clay_ElementDeclaration d = UIRect::buildDecl();

        return d;
    }

    void UIFoldableTreeItem::draw(ClaySdL3RendererData *data)
    {
        bool is_open = getComponentMutable<components::UIFoldableTreeItem>().open;
        auto child_container = getChildren()[1];
        auto &chContainer_comp = child_container.getComponentMutable<components::UI>();

        chContainer_comp.visible = is_open;
    }
} // namespace atmo::core::ecs::entities

ATMO_REGISTER_ENTITY(entities::UIFoldableTreeItem)
ATMO_REGISTER_COMPONENT(atmo::core::components::UIFoldableTreeItem)
