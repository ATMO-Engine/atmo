#pragma once

#include <functional>
#include <unordered_map>
#include "core/event/Aevent.hpp"

namespace atmo
{
    namespace core
    {
        namespace event
        {
            using EventId = uint32_t;

            /**
             * @brief Abstract base class for event listeners.
             */
            class AListener
            {
            public:
                /**
                 * @brief Virtual destructor for AListener.
                 */
                virtual ~AListener() = default;
                /**
                 * @brief Map of event IDs to their corresponding handler functions.
                 */
                std::unordered_map<EventId, std::function<void(event::AEvent *)>> handlers;
                /**
                 * @brief Callback function to handle events.
                 *
                 * @param event Pointer to the event to be handled.
                 */
                void callback(event::AEvent *event);
            };
        } // namespace event
    } // namespace core
} // namespace atmo
