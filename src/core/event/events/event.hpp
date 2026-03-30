#pragma once

#include <flecs.h>
#include <string_view>

namespace atmo::core::event::events
{
    class Event
    {
    public:
        virtual ~Event() = default;

        /**
         * @brief Check if the event has been consumed. If an event is consumed, it should not be propagated to any further listeners.
         *
         * @return true
         * @return false
         */
        bool isConsumed() const;

        /**
         * @brief Mark the event as consumed, preventing it from being propagated to any further listeners.
         *
         */
        void consume();

        static constexpr std::string_view FullName()
        {
            return "Event";
        }

    private:
        bool m_consumed = false;
    };
} // namespace atmo::core::event::events
