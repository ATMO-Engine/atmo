#pragma once

#include "core/resource/subresource.hpp"

namespace atmo
{
    namespace core
    {
        namespace resource
        {
            namespace resources
            {
                class Shape2d : public SubResource
                {
                public:
                    std::string_view name() const noexcept override;
                };
            }; // namespace resources
        } // namespace resource
    } // namespace core
} // namespace atmo
