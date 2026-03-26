#pragma once

#include "meta/component_meta.hpp"
#include "meta/meta_registry.hpp"

#define ATMO_REGISTER_COMPONENT(Type)                                \
    namespace                                                        \
    {                                                                \
        static const bool _ = [] {                                   \
            ::atmo::meta::MetaRegistry::get().register_type<Type>(); \
            return true;                                             \
        }();                                                         \
    }
