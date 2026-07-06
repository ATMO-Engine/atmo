#pragma once

#include <optional>
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

        /**
         * @brief Runtime accessor for the concrete type's registry name (its `FullName()`), needed because
         * `FullName()` itself is `static constexpr` and can't be called polymorphically through a base pointer.
         */
        virtual std::string_view getTypeName() const = 0;

        virtual std::string_view name() = 0;
        virtual std::string_view description() = 0;
        virtual std::string_view iconPath() = 0;

        virtual void init(atmo::core::ecs::entities::UI &container) = 0;
        virtual void createTools() = 0;

        virtual void save() = 0;
        virtual void load() = 0;

        bool hasFilePath() const
        {
            return p_file_path.has_value();
        }

        const std::optional<std::string> &filePath() const
        {
            return p_file_path;
        }

        void open(const std::string &path)
        {
            p_file_path = path;
            load();
        }

        void saveAs(const std::string &path)
        {
            p_file_path = path;
            save();
        }

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
        std::optional<std::string> p_file_path;
        std::vector<EditorTool> p_tools;

    private:
        std::vector<std::vector<EditorTool>::iterator> m_selected_tools;
    };
} // namespace atmo::editor
