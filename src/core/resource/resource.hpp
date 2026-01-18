#pragma once

#include <memory>
#include <string>

namespace atmo
{
    namespace core
    {
        namespace resource
        {
            template<typename T>
            class Resource
            {
            public:
                virtual ~Resource() = default;

                virtual std::shared_ptr<T> load(const std::string &path) = 0;
            };
        } // namespace resource
    } // namespace core
} // namespace atmo
