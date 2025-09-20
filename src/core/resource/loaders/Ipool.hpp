#pragma once

#include <any>
#include <string>
#include "core/resource/handle.hpp"

namespace atmo
{
    namespace core
    {
        namespace resource
        {
            class IPool
            {
            public:
                virtual ~IPool() = default;

                virtual const Handle create(const std::string &path) = 0;
                virtual std::any getFromHandle(const Handle &handle) = 0;
                virtual void destroy(const Handle &handle) = 0;
            };
        } // namespace resource
    } // namespace core
} // namespace atmo
