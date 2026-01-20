#pragma once

#include <cstdint>

namespace atmo
{
    namespace core
    {
        namespace resource
        {
            class IPoolGarbageCollector
            {
            public:
                virtual ~IPoolGarbageCollector() = default;
                virtual void collectGarbage(uint64_t currentFrame) = 0;
            };
        } // namespace resource
    } // namespace core
} // namespace atmo
