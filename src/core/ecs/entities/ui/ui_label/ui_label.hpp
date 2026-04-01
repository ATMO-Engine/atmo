#pragma once

#include <string>
#include "SDL3_ttf/SDL_ttf.h"
#include "core/ecs/entities/ui/ui.hpp"
#include "core/resource/handle.hpp"
#include "core/types.hpp"
#include "meta/meta.hpp"

namespace atmo::core::components
{
    struct UILabel {
        std::string text;
        float font_size = 14.0f;
        std::string font_path = "project://assets/fonts/Nunito.ttf";
        resource::Handle<TTF_Font> font_handle;
    };
} // namespace atmo::core::components

template <> struct atmo::meta::ComponentMeta<atmo::core::components::UILabel> {
    static constexpr const char *name = "Text";
    static constexpr const char *category = "UI";
    static constexpr auto fields = std::make_tuple(
        atmo::meta::field<&atmo::core::components::UILabel::text>("text"), atmo::meta::field<&atmo::core::components::UILabel::font_size>("font_size"),
        atmo::meta::field<&atmo::core::components::UILabel::font_path>("font_path"));
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
    };
} // namespace atmo::core::ecs::entities
