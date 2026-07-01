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

        std::string_view name() override
        {
            return "atmo.editors.texture_editor.name";
        }

        std::string_view description() override
        {
            return "atmo.editors.texture_editor.description";
        }

        std::string_view iconPath() override
        {
            return "project://assets/icons/brush.svg";
        }

        void init(atmo::core::ecs::entities::UI &container) override;
        void createTools() override;
    };
} // namespace atmo::editor
