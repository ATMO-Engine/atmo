#include "integrated_menu_bar.hpp"

#if !defined(ATMO_EXPORT)

#include <string>

#include "SDL3/SDL_keyboard.h"
#include "core/ecs/entities/ui/ui_button/ui_button.hpp"
#include "core/ecs/entities/ui/ui_dropdown_menu/ui_dropdown_menu.hpp"
#include "core/ecs/entities/ui/ui_label/ui_label.hpp"
#include "core/ecs/entities/ui/ui_layout.hpp"
#include "core/ecs/entities/ui/ui_rect/ui_rect.hpp"
#include "core/ecs/entity_registry.hpp"
#include "core/types.hpp"
#include "locale/locale_manager.hpp"

namespace atmo::editor
{
    namespace
    {
        using core::components::Layout;
        using SizingType = Layout::SizingAxis::SizingAxisType;

        constexpr float BAR_HEIGHT = 28.0f;
        constexpr float ITEM_MIN_WIDTH = 220.0f;
        constexpr float FONT_SIZE = 12.0f;
        constexpr int DROPDOWN_GROUP = 100;

        std::string translate(const std::string &key)
        {
            return std::string(locale::LocaleManager::GetTranslation(key));
        }

        std::string shortcutToString(const Shortcut &shortcut)
        {
            std::string text;
            if (shortcut.modifiers & SDL_KMOD_CTRL)
                text += "Ctrl+";
            if (shortcut.modifiers & SDL_KMOD_ALT)
                text += "Alt+";
            if (shortcut.modifiers & SDL_KMOD_SHIFT)
                text += "Shift+";
            if (shortcut.modifiers & SDL_KMOD_GUI)
                text += "Win+";
            text += SDL_GetKeyName(shortcut.key);
            return text;
        }

        std::shared_ptr<core::ecs::entities::UILabel> makeLabel(const std::string &text, const core::types::Color &color)
        {
            auto label = core::ecs::EntityRegistry::Create<core::ecs::entities::UILabel>("Entity::UI::UILabel");
            label->setText(text);
            label->setFontSize(FONT_SIZE);
            label->getComponentMutable<core::components::UI>().modulate = color;
            return label;
        }

        void addSeparator(core::ecs::entities::Entity parent)
        {
            auto separator = core::ecs::EntityRegistry::Create<core::ecs::entities::UIRect>("Entity::UI::UIRect");
            separator->getComponentMutable<core::components::UIRect>().color = core::types::Color("D4D4D8");
            auto &layout = separator->getComponentMutable<Layout>();
            layout.width.type = SizingType::GROW;
            layout.height.type = SizingType::FIXED;
            layout.height.size = Layout::SizingAxis::MinMax{ 1.0f, 1.0f };
            separator->setParent(parent);
        }

        void addCommandItem(core::ecs::entities::Entity parent, const Command &cmd, Commands &commands)
        {
            auto item = core::ecs::EntityRegistry::Create<core::ecs::entities::UIButton>("Entity::UI::UIRect::UIButton");
            auto &rect = item->getComponentMutable<core::components::UIRect>();
            rect.color = core::types::Color::TRANSPARENT_COL;
            rect.corner_radius = { 4, 4, 4, 4 };
            auto &layout = item->getComponentMutable<Layout>();
            layout.width.type = SizingType::GROW;
            layout.width.size = Layout::SizingAxis::MinMax{ ITEM_MIN_WIDTH, 0.0f };
            layout.child_alignment.vertical = Layout::ChildAlignment::Center;
            layout.padding = { 8, 8, 4, 4 };
            layout.child_gap = 24;

            makeLabel(translate(cmd.id), core::types::Color::BLACK)->setParent(*item);

            auto spacer = core::ecs::EntityRegistry::Create<core::ecs::entities::UI>("Entity::UI");
            spacer->getComponentMutable<Layout>().width.type = SizingType::GROW;
            spacer->setParent(*item);

            if (cmd.shortcut.has_value())
                makeLabel(shortcutToString(*cmd.shortcut), core::types::Color("94A3B8"))->setParent(*item);

            item->setParent(parent);

            auto setColor = [handle = item->getHandle()](bool active) {
                if (!handle.is_alive())
                    return;
                auto &item_rect = core::ecs::entities::UIRect(handle).getComponentMutable<core::components::UIRect>();
                item_rect.color = active ? core::types::Color("DBEAFE") : core::types::Color::TRANSPARENT_COL;
                item_rect.color.a = active ? 1.0f : 0.0f;
            };
            item->getSignal<>("MouseEntered").connect([setColor]() { setColor(true); });
            item->getSignal<>("MouseExited").connect([setColor]() { setColor(false); });
            item->getSignal<>("Released").connect([id = cmd.id, &commands]() { commands.execute(id); });
        }
    } // namespace

    void IntegratedMenuBar::build(core::ecs::entities::Window /*window_entity*/, Commands &commands, core::ecs::entities::UI container)
    {
        auto bar = core::ecs::EntityRegistry::Create<core::ecs::entities::UIRect>("Entity::UI::UIRect");
        bar->getComponentMutable<core::components::UIRect>().color = core::types::Color::WHITE;
        auto &bar_layout = bar->getComponentMutable<Layout>();
        bar_layout.width.type = SizingType::GROW;
        bar_layout.height.type = SizingType::FIXED;
        bar_layout.height.size = Layout::SizingAxis::MinMax{ BAR_HEIGHT, BAR_HEIGHT };
        bar_layout.direction = Layout::Direction::Horizontal;
        bar_layout.child_alignment.vertical = Layout::ChildAlignment::Center;
        bar_layout.padding = { 8, 8, 2, 2 };
        bar_layout.child_gap = 2;
        bar->setParent(container);

        for (auto &[category, cmds] : commands.byCategory()) {
            auto dropdown = core::ecs::EntityRegistry::Create<core::ecs::entities::UiDropdownMenu>("Entity::UI::UIRect::UIButton::UiDropdownMenu");
            auto &dropdown_comp = dropdown->getComponentMutable<core::components::UiDropdownMenu>();
            dropdown_comp.group = DROPDOWN_GROUP;
            dropdown_comp.open_side = core::components::UiDropdownMenu::OpenSide::BOTTOM;

            dropdown->setText(translate(category));
            dropdown->getLabel().setFontSize(FONT_SIZE);

            auto &dropdown_rect = dropdown->getComponentMutable<core::components::UIRect>();
            dropdown_rect.color = core::types::Color::TRANSPARENT_COL;
            dropdown_rect.corner_radius = { 4, 4, 4, 4 };
            auto &dropdown_layout = dropdown->getComponentMutable<Layout>();
            dropdown_layout.width.type = SizingType::FIT;
            dropdown_layout.height.type = SizingType::GROW;
            dropdown_layout.child_alignment.vertical = Layout::ChildAlignment::Center;
            dropdown_layout.padding = { 10, 10, 0, 0 };

            auto content = dropdown->getContent();
            auto &content_rect = content.getComponentMutable<core::components::UIRect>();
            content_rect.color = core::types::Color("F5F5F5");
            content_rect.color.a = 1.0f;

            for (const Command *cmd : cmds) {
                if (!cmd)
                    addSeparator(content);
                else
                    addCommandItem(content, *cmd, commands);
            }

            dropdown->setParent(*bar);
        }
    }
} // namespace atmo::editor

#endif
