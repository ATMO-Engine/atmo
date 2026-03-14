#pragma once

#include <flecs.h>
#include <string_view>

namespace atmo::core::event::events
{
    /**
     * @brief Base class for all events in the system. This class provides basic functionality for event consumption, allowing listeners to mark an event as
     * consumed to prevent further propagation.
     *
     */
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

        /**
         * @brief Get the Full Name of the event, used for registration and dispatching.
         *
         * @return constexpr std::string_view
         */
        static constexpr std::string_view FullName()
        {
            return "Event";
        }

    private:
        /**
         * @brief Boolean flag indicating whether the event has been consumed. If true, the event should not be propagated to any further listeners.
         *
         */
        bool m_consumed = false;
    };
} // namespace atmo::core::event::events
