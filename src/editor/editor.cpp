#include "editor.hpp"
#include "project/file_system.hpp"

namespace atmo
{
    namespace editor
    {
        Editor::Editor(atmo::core::Engine *engine, const std::string &project_path) : m_engine(engine), m_project_path(project_path) {}

        void Editor::init()
        {
            // auto test = project::FileSystem::OpenFile("project://translation/en-US/atmo.json");

            // std::string content = test.readAll();
            // std::cout.write(content.data(), content.size());
        }

        void atmo::editor::Editor::loop()
        {
            // Placeholder for editor loop logic
        }
    } // namespace editor
} // namespace atmo
