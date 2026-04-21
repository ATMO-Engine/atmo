#pragma once

#include <string>
#include <string_view>
#include "SDL3_ttf/SDL_ttf.h"
#include "core/ecs/entities/ui/ui.hpp"
#include "core/resource/handle.hpp"
#include "core/types.hpp"
#include "meta/meta.hpp"

namespace atmo::core::components
{
    struct UILabel {
        // Clay_TextElementConfigWrapMode wrapMode{
        //     CLAY_TEXT_WRAP_WORDS(default),
        //     CLAY_TEXT_WRAP_NEWLINES,
        //     CLAY_TEXT_WRAP_NONE,
        // };
        // Clay_TextAlignment textAlignment{
        //     CLAY_TEXT_ALIGN_LEFT(default),
        //     CLAY_TEXT_ALIGN_CENTER,
        //     CLAY_TEXT_ALIGN_RIGHT,
        // };

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
        float font_size = 24.0f;
        std::string font_path = "project://assets/fonts/Nunito/Nunito.ttf";
        resource::Handle<TTF_Font> font_handle;
        uint16_t letter_spacing = 0;
        uint16_t line_height = 0;
        TTF_Text *ttf_text = nullptr;

        WrapMode wrap_mode = WrapMode::WRAP_WORDS;
        TextAlignment text_alignment = TextAlignment::ALIGN_LEFT;
    };
} // namespace atmo::core::components

template <> struct atmo::meta::ComponentMeta<atmo::core::components::UILabel> {
    static constexpr const char *name = "Text";
    static constexpr const char *category = "UI";
    static constexpr auto fields = std::make_tuple(
        atmo::meta::field<&atmo::core::components::UILabel::text>("text"), atmo::meta::field<&atmo::core::components::UILabel::font_size>("font_size"),
        atmo::meta::field<&atmo::core::components::UILabel::font_path>("font_path"),
        atmo::meta::field<&atmo::core::components::UILabel::letter_spacing>("letter_spacing"),
        atmo::meta::field<&atmo::core::components::UILabel::line_height>("line_height"),
        atmo::meta::field<&atmo::core::components::UILabel::wrap_mode>("wrap_mode"),
        atmo::meta::field<&atmo::core::components::UILabel::text_alignment>("text_alignment"));
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
        std::string_view getText();

        void draw(ClaySdL3RendererData *data) override;
    };
} // namespace atmo::core::ecs::entities
