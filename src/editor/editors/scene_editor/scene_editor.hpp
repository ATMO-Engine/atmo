#pragma once

#include "editor/editor_registry.hpp"
#include "editor/editors/editor.hpp"

namespace atmo::editor
{
    class SceneEditor : public EditorRegistry::Registrable<SceneEditor, Editor>
    {
    public:
        using EditorRegistry::Registrable<SceneEditor, Editor>::Registrable;

        static constexpr std::string_view LocalName()
        {
            return "SceneEditor";
        }

        static std::string_view Name()
        {
            return "atmo.editors.scene_editor.name";
        }

        static std::string_view Description()
        {
            return "atmo.editors.scene_editor.description";
        }

        static std::string_view IconPath()
        {
            return "project://atmo.png";
        }

        void init(atmo::core::ecs::entities::UI &container);
    };
} // namespace atmo::editor
