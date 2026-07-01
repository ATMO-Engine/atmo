#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include "SDL3/SDL_pixels.h"
#include "SDL3/SDL_render.h"
#include "SDL3_ttf/SDL_ttf.h"
#include "core/ecs/entities/ui/ui.hpp"
#include "core/resource/resource_ref.hpp"
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

        struct TextRenderCache {
            TTF_Text *ttf_text = nullptr;
            SDL_Texture *texture = nullptr;
            bool dirty = true;
            bool engine_bound = false;

            SDL_Color last_color = {};
            uint16_t last_font_size = 0;
            float last_dpi_x = 0.f;
            float last_dpi_y = 0.f;
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

        std::unique_ptr<resource::ResourceRef<TTF_Font>> m_res;
        std::unique_ptr<TextRenderCache> m_render_cache;

        std::string m_prev_text;
        std::string m_prev_font_path;
        uint16_t m_prev_font_size = 0;
        int m_prev_font_style = -1;
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

        void draw(ClaySdL3RendererData *data) override;
    };
} // namespace atmo::core::ecs::entities
