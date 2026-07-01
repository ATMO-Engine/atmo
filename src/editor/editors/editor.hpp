#pragma once

#include <string>
#include <string_view>
#include <vector>
#include "core/ecs/entities/entity.hpp"
#include "core/ecs/entities/ui/ui.hpp"

namespace atmo::editor
{
    class Editor
    {
    public:
        virtual ~Editor() = default;

        static constexpr std::string_view FullName()
        {
            return "Editor";
        }

        virtual std::string_view name() = 0;
        virtual std::string_view description() = 0;
        virtual std::string_view iconPath() = 0;

        virtual void init(atmo::core::ecs::entities::UI &container) = 0;
        virtual void createTools() = 0;

        bool isToggleToolSelected(std::string_view tool_name);

        struct EditorTool {
            enum class Type {
                BUTTON,
                TOGGLE,
                TOGGLE_GROUP
            };

            Type type;
            std::string_view name;
            std::string_view icon_path;

            std::optional<std::function<void(bool active)>> callback;
        };

    protected:
        std::vector<EditorTool> p_tools;

    private:
        std::vector<std::vector<EditorTool>::iterator> m_selected_tools;
    };
} // namespace atmo::editor
