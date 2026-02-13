#pragma once

#include "core/resource/subresource_registry.hpp"
#include "core/resource/subresources/subresource.hpp"

namespace atmo
{
    namespace core
    {
        namespace resource
        {
            namespace resources
            {
                class Shape2d : public SubResourceRegistry::Registrable<Shape2d, SubResource>
                {
                public:
                    using SubResourceRegistry::Registrable<Shape2d, SubResource>::Registrable;

                    static constexpr std::string_view LocalName()
                    {
                        return "Shape2d";
                    }
                };
            }; // namespace resources
        } // namespace resource
    } // namespace core
} // namespace atmo
