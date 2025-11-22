#pragma once

#include <cstdint>

namespace atmo
{
    namespace core
    {
        namespace event
        {
            using EventId = uint32_t;
            class IEvent
            {
            public:
                virtual ~IEvent() = default;
                bool isConsumed() const;
                void consume();

                EventId id;

            private:
                bool m_consumed = false;
            };
        } // namespace event
    } // namespace core
} // namespace atmo
