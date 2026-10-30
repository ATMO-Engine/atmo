#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include "core/ecs/entities/ui/ui.hpp"
#include "core/resource/font.hpp"
#include "core/types.hpp"
#include "meta/field_descriptor.hpp"
#include "meta/meta.hpp"

namespace atmo::core::components
{
    struct UILabel {
        enum class WrapMode {
            WRAP_WORDS,
            WRAP_NEWLINES,
            WRAP_NONE
        };

        enum class TextAlignment {
            ALIGN_LEFT,
            ALIGN_CENTER,
            ALIGN_RIGHT
        };

        std::string text;
        std::uint16_t font_size = 24;
        std::string font_path = "project://assets/fonts/Nunito/Nunito.ttf";
        std::uint16_t letter_spacing = 0;
        std::uint16_t line_height = 0;
        bool font_bold = false;
        bool font_italic = false;

        WrapMode wrap_mode = WrapMode::WRAP_WORDS;
        TextAlignment text_alignment = TextAlignment::ALIGN_LEFT;

        std::shared_ptr<resource::Font> m_font;
        std::string m_loaded_font_path;
    };
} // namespace atmo::core::components

template <> struct atmo::meta::ComponentMeta<atmo::core::components::UILabel> {
    static constexpr const char *name = "Text";
    static constexpr const char *category = "UI";
    static constexpr auto fields = std::make_tuple(
        atmo::meta::field<&atmo::core::components::UILabel::text>("text"), atmo::meta::field<&atmo::core::components::UILabel::font_size>("font_size"),
        atmo::meta::field<&atmo::core::components::UILabel::font_path>("font_path").withWidget("file_path"),
        atmo::meta::field<&atmo::core::components::UILabel::letter_spacing>("letter_spacing"),
        atmo::meta::field<&atmo::core::components::UILabel::line_height>("line_height"),
        atmo::meta::field<&atmo::core::components::UILabel::wrap_mode>("wrap_mode"),
        atmo::meta::field<&atmo::core::components::UILabel::text_alignment>("text_alignment"),
        atmo::meta::field<&atmo::core::components::UILabel::font_bold>("font_bold"),
        atmo::meta::field<&atmo::core::components::UILabel::font_italic>("font_italic"));
};

namespace atmo::core::ecs::entities
{
    class UILabel : public EntityRegistry::Registrable<UILabel, UI>
    {
    public:
        using EntityRegistry::Registrable<UILabel, UI>::Registrable;

        static void RegisterSystems(flecs::world *world);

        void initialize();

        static constexpr std::string_view LocalName()
        {
            return "UILabel";
        }

        void setFontPath(const std::string &path);
        std::string_view getFontPath() const noexcept;

        void setText(const std::string &text);
        std::string_view getText() const noexcept;

        void setFontSize(std::uint16_t font_size);
        std::uint16_t getFontSize() const noexcept;

        void setFontBold(bool bold);
        void setFontItalic(bool italic);

        void setLetterSpacing(std::uint16_t spacing);
        std::uint16_t getLetterSpacing() const noexcept;

        void setLineHeight(std::uint16_t height);
        std::uint16_t getLineHeight() const noexcept;

        void setWrapMode(components::UILabel::WrapMode mode);
        components::UILabel::WrapMode getWrapMode() const noexcept;

        void setTextAlignment(components::UILabel::TextAlignment alignment);
        components::UILabel::TextAlignment getTextAlignment() const noexcept;

        void draw() override;
    };
} // namespace atmo::core::ecs::entities
