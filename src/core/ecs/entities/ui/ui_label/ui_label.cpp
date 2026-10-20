#include "ui_label.hpp"
#include "clay.h"
#include "core/ecs/entities/ui/ui.hpp"
#include "core/ecs/entities/window/window.hpp"
#include "core/resource/resource_manager.hpp"
#include "core/types.hpp"
#include "impl/clay_types.hpp"
#include "locale/locale_manager.hpp"
#include "meta/auto_register.hpp"
#include "spdlog/spdlog.h"

namespace atmo::core::ecs::entities
{
    void UILabel::RegisterSystems(flecs::world *world)
    {
        world->system<components::UILabel>("UILabel_sync").kind(flecs::PreUpdate).each([](flecs::entity, components::UILabel &label) {
            if (label.font_path == label.m_loaded_font_path)
                return;

            label.m_loaded_font_path = label.font_path;
            label.m_font = nullptr;
            if (label.font_path.empty())
                return;

            try {
                label.m_font = resource::ResourceManager::GetInstance().getResource<resource::Font>(label.font_path);
            } catch (const std::exception &e) {
                spdlog::error("UILabel: {}", e.what());
            }
        });

        world->observer<components::UILabel>("UILabel_remove").event(flecs::OnRemove).each([](flecs::entity, components::UILabel &label) {
            label.m_font = nullptr;
        });
    }

    void UILabel::initialize()
    {
        UI::initialize();

        setComponent<components::UILabel>({});
    }

    void UILabel::setFontPath(const std::string &path)
    {
        getComponentMutable<components::UILabel>().font_path = path;
    }

    std::string_view UILabel::getFontPath() const noexcept
    {
        return getComponent<components::UILabel>().font_path;
    }

    void UILabel::setText(const std::string &text)
    {
        getComponentMutable<components::UILabel>().text = locale::LocaleManager::GetTranslation(text);
    }

    std::string_view UILabel::getText() const noexcept
    {
        return getComponent<components::UILabel>().text;
    }

    void UILabel::setFontSize(std::uint16_t font_size)
    {
        getComponentMutable<components::UILabel>().font_size = font_size;
    }

    std::uint16_t UILabel::getFontSize() const noexcept
    {
        return getComponent<components::UILabel>().font_size;
    }

    void UILabel::setFontBold(bool bold)
    {
        getComponentMutable<components::UILabel>().font_bold = bold;
    }

    void UILabel::setFontItalic(bool italic)
    {
        getComponentMutable<components::UILabel>().font_italic = italic;
    }

    void UILabel::setLetterSpacing(std::uint16_t spacing)
    {
        getComponentMutable<components::UILabel>().letter_spacing = spacing;
    }

    std::uint16_t UILabel::getLetterSpacing() const noexcept
    {
        return getComponent<components::UILabel>().letter_spacing;
    }

    void UILabel::setLineHeight(std::uint16_t height)
    {
        getComponentMutable<components::UILabel>().line_height = height;
    }

    std::uint16_t UILabel::getLineHeight() const noexcept
    {
        return getComponent<components::UILabel>().line_height;
    }

    void UILabel::setWrapMode(components::UILabel::WrapMode mode)
    {
        getComponentMutable<components::UILabel>().wrap_mode = mode;
    }

    components::UILabel::WrapMode UILabel::getWrapMode() const noexcept
    {
        return getComponent<components::UILabel>().wrap_mode;
    }

    void UILabel::setTextAlignment(components::UILabel::TextAlignment alignment)
    {
        getComponentMutable<components::UILabel>().text_alignment = alignment;
    }

    components::UILabel::TextAlignment UILabel::getTextAlignment() const noexcept
    {
        return getComponent<components::UILabel>().text_alignment;
    }

    void UILabel::draw()
    {
        const auto &ui = getComponent<components::UI>();
        const auto &label = getComponent<components::UILabel>();

        if (!label.m_font)
            return;

        const std::uint16_t font_flags = (label.font_bold ? clay_font_flags::bold : 0) | (label.font_italic ? clay_font_flags::italic : 0);
        Clay_String str = Clay_String{ .isStaticallyAllocated = false, .length = static_cast<int32_t>(label.text.size()), .chars = label.text.data() };
        CLAY_TEXT(
            str,
            CLAY_TEXT_CONFIG(
                { .userData = static_cast<void *>(label.m_font.get()),
                  .textColor = (types::Color::WHITE * ui.modulate).toFloat<Clay_Color>(255),
                  .fontId = font_flags,
                  .fontSize = static_cast<uint16_t>(label.font_size),
                  .letterSpacing = label.letter_spacing,
                  .lineHeight = label.line_height,
                  .wrapMode = static_cast<Clay_TextElementConfigWrapMode>(label.wrap_mode),
                  .textAlignment = static_cast<Clay_TextAlignment>(label.text_alignment) }));
    }
} // namespace atmo::core::ecs::entities

ATMO_REGISTER_ENTITY(entities::UILabel);
ATMO_REGISTER_COMPONENT(atmo::core::components::UILabel)
