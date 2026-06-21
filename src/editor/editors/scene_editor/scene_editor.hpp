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
            return "project://assets/icons/gamepad.svg";
        }

        void init(atmo::core::ecs::entities::UI &container) override;

        void
        sceneEntityFodableTreeinit(core::ecs::entities::Entity entity, core::ecs::entities::Entity parent, core::ecs::entities::Entity component_container);

        flecs::entity getSelectedEntity();
        void setSelectedEntity(flecs::entity new_slected_entity);

    private:
        flecs::entity m_selected_entity;
    };
} // namespace atmo::editor
