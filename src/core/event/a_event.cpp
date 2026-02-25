#include "a_event.hpp"

namespace atmo
{
    namespace core
    {
        namespace event
        {
            bool AEvent::isConsumed() const
            {
                return m_consumed;
            }

            void AEvent::consume()
            {
                m_consumed = true;
            }
        } // namespace event
    } // namespace core
} // namespace atmo
