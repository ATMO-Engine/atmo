#pragma once

#include "core/registry/hierarchic_registry.hpp"
#include "core/resource/subresources/subresource.hpp"

#define REGISTER_SUBRESOURCE(res)                     \
    namespace                                         \
    {                                                 \
        static int _ = [] {                           \
            using namespace atmo::core::resource;     \
            SubResourceRegistry::RegisterType<res>(); \
            return 0;                                 \
        }();                                          \
    }

namespace atmo::core::resource
{
    class SubResourceRegistry : public registry::HierarchicRegistry<SubResourceRegistry, SubResource>
    {
    public:
        template <typename Type> static std::unique_ptr<SubResource> Factorize()
        {
            return std::make_unique<Type>();
        }
    };
} // namespace atmo::core::resource
