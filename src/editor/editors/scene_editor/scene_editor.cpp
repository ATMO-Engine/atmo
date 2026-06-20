#include "scene_editor.hpp"
#include "editor/editor_registry.hpp"

namespace atmo::editor
{
    void SceneEditor::init(atmo::core::ecs::entities::UI &container)
    {
        return;
    }
} // namespace atmo::editor

ATMO_REGISTER_EDITOR(atmo::editor::SceneEditor);
