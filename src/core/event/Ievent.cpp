#include "Ievent.hpp"

namespace atmo
{
    namespace core
    {
        namespace event
        {
            bool IEvent::isConsumed() const
            {
                return m_consumed;
            }

            void IEvent::consume()
            {
                m_consumed = true;
            }
        } // namespace event
    } // namespace core
} // namespace atmo
