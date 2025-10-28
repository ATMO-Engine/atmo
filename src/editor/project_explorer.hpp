#pragma once

#include "core/engine.hpp"

namespace atmo
{
    namespace editor
    {
#if !defined(ATMO_EXPORT)
        class ProjectExplorer
        {
        private:
            atmo::core::Engine *m_engine;
            std::string m_selected_path;

        public:
            ProjectExplorer(atmo::core::Engine *engine);
            ~ProjectExplorer() = default;

            void loop();
            inline const std::string getSelectedPath() const
            {
                return m_selected_path;
            }
        };
#endif
    } // namespace editor
} // namespace atmo
