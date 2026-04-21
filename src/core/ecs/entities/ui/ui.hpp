#pragma once

#include <string>
#include "core/types.hpp"
#include "meta/meta.hpp"

namespace atmo::core::components::UI
{
    struct UI {
        bool visible{ true };
        types::ColorRGBA modulate{ 1.0f, 1.0f, 1.0f, 1.0f };
        types::ColorRGBA self_modulate{ 1.0f, 1.0f, 1.0f, 1.0f };
    };

    struct Text {
        std::string content;
    };
} // namespace atmo::core::components::UI

template <> struct atmo::meta::ComponentMeta<atmo::core::components::UI::UI> {
    static constexpr const char *name = "UI";
    static constexpr const char *category = "UI";
    static constexpr auto fields = std::make_tuple(
        atmo::meta::field<&atmo::core::components::UI::UI::visible>("visible"),
        atmo::meta::field<&atmo::core::components::UI::UI::modulate>("modulate").withWidget("color_picker"),
        atmo::meta::field<&atmo::core::components::UI::UI::self_modulate>("self_modulate").withWidget("color_picker"));
};

template <> struct atmo::meta::ComponentMeta<atmo::core::components::UI::Text> {
    static constexpr const char *name = "Text";
    static constexpr const char *category = "UI";
    static constexpr auto fields = std::make_tuple(atmo::meta::field<&atmo::core::components::UI::Text::content>("content"));
};
