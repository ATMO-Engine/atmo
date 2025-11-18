#pragma once

#include <any>

namespace atmo
{
    namespace core
    {
        namespace event
        {
            class IEvent
            {
            public:
                virtual ~IEvent() = default;
                bool isConsumed() const;
                void consume();

            private:
                bool m_consumed = false;
            };
        } // namespace event
    } // namespace core
} // namespace atmo
