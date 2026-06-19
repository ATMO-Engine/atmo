#pragma once

#include "editor/editor_registry.hpp"
#include "editor/editors/editor.hpp"

namespace atmo::editor
{
    class TextureEditor : public EditorRegistry::Registrable<TextureEditor, Editor>
    {
    public:
        using EditorRegistry::Registrable<TextureEditor, Editor>::Registrable;

        static constexpr std::string_view LocalName()
        {
            return "TextureEditor";
        }

        static std::string_view Name()
        {
            return "atmo.editors.texture_editor.name";
        }

        static std::string_view Description()
        {
            return "atmo.editors.texture_editor.description";
        }

        static std::string_view IconPath()
        {
            return "project://atmo.png";
        }

        void init(atmo::core::ecs::entities::UI &container);
    };
} // namespace atmo::editor
