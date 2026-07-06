#pragma once

#include "meta/component_meta.hpp"
#include "meta/meta_registry.hpp"


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

#define ATMO_REGISTER_COMPONENT_NO_FLECS_IMPL(Type, uniqueName)               \
    namespace                                                                 \
    {                                                                         \
        static const bool uniqueName = [] {                                   \
            ::atmo::meta::MetaRegistry::Instance().registerType<Type>(false); \
            return true;                                                      \
        }();                                                                  \
    }

#define ATMO_REGISTER_COMPONENT_NO_FLECS(Type) ATMO_REGISTER_COMPONENT_NO_FLECS_IMPL(Type, ATMO_CONCAT(_meta_reg_, __COUNTER__))
