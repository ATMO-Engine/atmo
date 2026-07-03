#pragma once

#include <memory>
#include <string>

#include "core/ecs/entities/ui/ui_button/ui_button.hpp"
#include "editor/editor_registry.hpp"
#include "editor/editors/editor.hpp"
#include "editor/editors/scene_editor/editor_scene_context.hpp"

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
            return "project://assets/icons/gamepad.svg";
        }

        void init(atmo::core::ecs::entities::UI &container) override;
        void createTools() override;

        void
        sceneEntityFodableTreeinit(core::ecs::entities::Entity entity, core::ecs::entities::Entity parent, core::ecs::entities::Entity component_container);
        void createNewEntitySelectionPopup(core::ecs::entities::Entity parent);
        core::ecs::entities::UIButton makeEntityCreationButton(const std::string &entity_id);
        void createAddEntityFodableTree(EditorRegistry::EntityTree &tree, core::ecs::entities::Entity parentUI);
        flecs::entity getSelectedEntity();
        void setSelectedEntity(flecs::entity new_slected_entity);

        EditorSceneContext *getSceneContext() const
        {
            return m_scene_ctx.get();
        }

    private:
        flecs::entity m_selected_entity;
        flecs::entity m_viewport_image;
        std::unique_ptr<EditorSceneContext> m_scene_ctx;
        std::string m_scene_file_path;
        std::vector<std::function<void()>> m_inspector_update_fns;
    };
} // namespace atmo::editor
