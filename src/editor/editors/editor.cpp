#include "editor.hpp"

namespace atmo::editor
{
    bool Editor::isToggleToolSelected(std::string_view tool_name)
    {
        for (auto it : m_selected_tools)
            if ((it->type == EditorTool::Type::TOGGLE || it->type == EditorTool::Type::TOGGLE_GROUP) && it->name == tool_name)
                return true;

        return false;
    }
} // namespace atmo::editor
