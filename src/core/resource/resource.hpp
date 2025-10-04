#pragma once

#include <any>
#include <string>

namespace atmo
{
    namespace core
    {
        namespace resource
        {
            template<typename T>
            struct LoaderExtension;

            class Resource
            {
            public:
                virtual ~Resource() = default;

                virtual void load(const std::string &path) = 0;
                virtual void destroy() = 0;

                virtual std::any get() = 0;
            };
        } // namespace resource
    } // namespace core
} // namespace atmo
