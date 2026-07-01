#pragma once

#include "core/registry/hierarchic_registry.hpp"
#include "editors/editor.hpp"

#define ATMO_REGISTER_EDITOR(edt)                \
    namespace                                    \
    {                                            \
        static int _ = [] {                      \
            using namespace atmo::editor;        \
            EditorRegistry::RegisterType<edt>(); \
            return 0;                            \
        }();                                     \
    }

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
