#pragma once

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

                virtual T *load(const std::string &path) = 0;
                virtual void destroy(T *res) = 0;
            };
        } // namespace resource
    } // namespace core
} // namespace atmo
