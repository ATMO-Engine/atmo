#pragma once

#include "meta/component_meta.hpp"
#include "meta/meta_registry.hpp"

#define ATMO_REGISTER_COMPONENT_IMPL(Type, counter)                      \
    namespace                                                            \
    {                                                                    \
        static const bool _meta_reg_##counter = [] {                     \
            ::atmo::meta::MetaRegistry::Instance().registerType<Type>(); \
            return true;                                                 \
        }();                                                             \
    }

#define ATMO_REGISTER_COMPONENT(Type) ATMO_REGISTER_COMPONENT_IMPL(Type, __COUNTER__)
