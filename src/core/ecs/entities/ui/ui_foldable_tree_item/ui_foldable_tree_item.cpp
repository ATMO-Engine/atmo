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

        auto openbox = core::ecs::EntityRegistry::Create<core::ecs::entities::UICheckBox>("Entity::UI::UIRect::UICheckBox");
        auto &openbox_layout = openbox->getComponentMutable<core::components::Layout>();
        openbox_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::PERCENT;
        openbox_layout.width.size = 0.1f;
        openbox_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::PERCENT;
        openbox_layout.height.size = 1.0f;
        openbox->setParent(*this);
        openbox->getSignal<core::ecs::entities::UICheckBox &>("Clicked").connect([parent = this](core::ecs::entities::UICheckBox &chBox) {
            auto &parentComp = parent->getComponentMutable<core::components::UIFoldableTreeItem>();
            auto &chBoxComp = chBox.getComponentMutable<core::components::UICheckBox>();

            parentComp.open = chBoxComp.trigger;
        });

        auto child_container = core::ecs::EntityRegistry::Create<core::ecs::entities::UIRect>("Entity::UI::UIRect");
        auto &child_container_layout = child_container->getComponentMutable<core::components::Layout>();
        child_container_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::FIT;
        child_container_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::FIT;
        child_container_layout.direction = core::components::Layout::Direction::Vertical;
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
        auto child_container = getChild("child_container");
        auto &chContainer_comp = child_container.getComponentMutable<components::UI>();

        chContainer_comp.visible = is_open;
    }
} // namespace atmo::core::ecs::entities

ATMO_REGISTER_ENTITY(entities::UIFoldableTreeItem)
ATMO_REGISTER_COMPONENT(atmo::core::components::UIFoldableTreeItem)
