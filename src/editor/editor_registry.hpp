#pragma once

#include "core/registry/hierarchic_registry.hpp"
#include "editors/editor.hpp"

namespace atmo::editor
{
    class EditorRegistry : public core::registry::HierarchicRegistry<EditorRegistry, editor::Editor>
    {
    public:
        template <typename Type> static editor::Editor *Factorize()
        {
            return new Type();
        }
    };
} // namespace atmo::editor
