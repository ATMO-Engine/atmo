#pragma once

#include "meta/component_meta.hpp"
#include "meta/meta_registry.hpp"

// Two levels of indirection are required so __COUNTER__ is expanded to its numeric value before being pasted
// into the anonymous-namespace variable name — pasting it directly (a single-macro `a##__COUNTER__`) leaves the
// literal token "__COUNTER__" in the name, which is harmless when a macro is only invoked once per file (the
// prior behavior here) but collides if the same macro is invoked more than once in one translation unit.
#define ATMO_CONCAT_IMPL(a, b) a##b
#define ATMO_CONCAT(a, b) ATMO_CONCAT_IMPL(a, b)

#define ATMO_REGISTER_COMPONENT_IMPL(Type, uniqueName)                   \
    namespace                                                            \
    {                                                                    \
        static const bool uniqueName = [] {                              \
            ::atmo::meta::MetaRegistry::Instance().registerType<Type>(); \
            return true;                                                 \
        }();                                                             \
    }

#define ATMO_REGISTER_COMPONENT(Type) ATMO_REGISTER_COMPONENT_IMPL(Type, ATMO_CONCAT(_meta_reg_, __COUNTER__))

#define ATMO_REGISTER_COMPONENT_NO_FLECS_IMPL(Type, uniqueName)                 \
    namespace                                                                   \
    {                                                                           \
        static const bool uniqueName = [] {                                    \
            ::atmo::meta::MetaRegistry::Instance().registerType<Type>(false);   \
            return true;                                                       \
        }();                                                                    \
    }

#define ATMO_REGISTER_COMPONENT_NO_FLECS(Type) ATMO_REGISTER_COMPONENT_NO_FLECS_IMPL(Type, ATMO_CONCAT(_meta_reg_, __COUNTER__))
