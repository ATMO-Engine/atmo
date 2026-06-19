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

        std::string_view name() override
        {
            return "atmo.editors.scene_editor.name";
        }

        std::string_view description() override
        {
            return "atmo.editors.scene_editor.description";
        }

        std::string_view iconPath() override
        {
            return "project://atmo.png";
        }

        void init(atmo::core::ecs::entities::UI &container) override;
    };
} // namespace atmo::editor
