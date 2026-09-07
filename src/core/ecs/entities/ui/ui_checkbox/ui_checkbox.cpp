#include "ui_checkbox.hpp"
#include "clay.h"
#include "core/ecs/entities/entity.hpp"
#include "core/ecs/entities/ui/ui.hpp"
#include "core/ecs/entities/ui/ui_button/ui_button.hpp"
#include "core/ecs/entities/ui/ui_image/ui_image.hpp"
#include "core/ecs/entities/ui/ui_label/ui_label.hpp"
#include "core/ecs/entities/ui/ui_layout.hpp"
#include "core/ecs/entity_registry.hpp"
#include "core/event/events/ui_event/hover_event/hover_event.hpp"
#include "meta/auto_register.hpp"
#include "spdlog/spdlog.h"

namespace atmo::core::ecs::entities
{
    void UICheckBox::RegisterSystems(flecs::world *world) {}

    void UICheckBox::initialize()
    {
        UIButton::initialize();
        setComponent<components::UICheckBox>({});

        getComponentMutable<core::components::UIButton>().toggle = true;
        auto checkBox_border = core::ecs::EntityRegistry::Create<core::ecs::entities::UIImage>("Entity::UI::UIImage");
        auto &checkBox_border_UIcomp = checkBox_border->getComponentMutable<core::components::UI>();
        auto &checkBox_border_Layoutcomp = checkBox_border->getComponentMutable<core::components::Layout>();

        checkBox_border->setTexturePath("project://assets/icons/square.svg");
        checkBox_border_UIcomp.modulate = core::types::Color::BLACK;
        checkBox_border_Layoutcomp.width.type = core::components::Layout::SizingAxis::SizingAxisType::FIXED;
        checkBox_border_Layoutcomp.width.size = core::components::Layout::SizingAxis::MinMax{ 18.0f, 18.0f };
        checkBox_border_Layoutcomp.height.type = core::components::Layout::SizingAxis::SizingAxisType::FIXED;
        checkBox_border_Layoutcomp.height.size = core::components::Layout::SizingAxis::MinMax{ 18.0f, 18.0f };
        checkBox_border_Layoutcomp.child_alignment = { core::components::Layout::ChildAlignment::Center, core::components::Layout::ChildAlignment::Center };
        checkBox_border->setParent(*this);

        auto checkBox_icon = core::ecs::EntityRegistry::Create<core::ecs::entities::UIImage>("Entity::UI::UIImage");
        auto &checkBox_icon_UIcomp = checkBox_icon->getComponentMutable<core::components::UI>();
        auto &checkBox_icon_Layoutcomp = checkBox_icon->getComponentMutable<core::components::Layout>();

        checkBox_icon->setTexturePath("project://assets/icons/check.svg");
        checkBox_icon_UIcomp.modulate = core::types::Color::BLACK;
        checkBox_icon_Layoutcomp.width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        checkBox_icon_Layoutcomp.height.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        checkBox_icon->setParent(*checkBox_border);

        auto handle = p_handle;
        getSignal<bool>("Toggle").connect([handle](bool new_state) {
            if (!handle.is_alive()) {
                return;
            }
            UICheckBox chBox(handle);
            if (!chBox.getComponent<core::components::UICheckBox>().default_texture)
                return;
            auto &checkBoxIcon = chBox.getChildren()[0].getChildren()[0].getComponentMutable<core::components::UI>();

            checkBoxIcon.visible = new_state;
        });
    }

    void UICheckBox::removeTexture()
    {
        getComponentMutable<core::components::UICheckBox>().default_texture = false;
        getChildren()[0].getComponentMutable<core::components::UI>().visible = false;
    }

    Clay_ElementDeclaration UICheckBox::buildDecl()
    {
        Clay_ElementDeclaration d = UIRect::buildDecl();

        return d;
    }

    void UICheckBox::draw(ClaySdL3RendererData *data)
    {
        UIButton::draw(data);
    }
} // namespace atmo::core::ecs::entities

ATMO_REGISTER_ENTITY(entities::UICheckBox);
ATMO_REGISTER_COMPONENT(atmo::core::components::UICheckBox)
