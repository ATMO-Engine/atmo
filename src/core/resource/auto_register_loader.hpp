#pragma once

#include "core/resource/resource_loader_registry.hpp"
#include "meta/auto_register.hpp"

#define ATMO_REGISTER_RESOURCE_LOADER_IMPL(Type, LoaderClass, uniqueName, ...)                                             \
    namespace                                                                                                              \
    {                                                                                                                      \
        static const bool uniqueName = [] {                                                                                \
            ::atmo::core::resource::ResourceLoaderRegistry::Instance().registerLoader<Type, LoaderClass>({ __VA_ARGS__ }); \
            return true;                                                                                                   \
        }();                                                                                                               \
    }

#define ATMO_REGISTER_RESOURCE_LOADER(Type, LoaderClass, ...) \
    ATMO_REGISTER_RESOURCE_LOADER_IMPL(Type, LoaderClass, ATMO_CONCAT(_res_loader_reg_, __COUNTER__), __VA_ARGS__)
