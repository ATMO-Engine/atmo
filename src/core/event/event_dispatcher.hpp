#pragma once

#include <algorithm>
#include <cstdint>
#include <typeinfo>
#include <unordered_map>
#include <vector>

#include "Aevent_listener.hpp"

namespace atmo
{
    namespace core
    {
        namespace event
        {
            /**
             * @brief Type alias for event identifiers.
             */
            using EventId = uint32_t;
            /**
             * @brief Generates a unique event ID for the given event type.
             *
             * @tparam EventType The type of the event.
             * @return A unique EventId for the event type.
             */
            template <typename EventType> static EventId event_id()
            {
                static const EventId id = static_cast<EventId>(typeid(EventType).hash_code());
                return id;
            }

            /**
             * @brief Event dispatcher ( singletone ) for managing event subscriptions and dispatching.
             */
            class EventDispatcher
            {
            public:
                /**
                 * @brief Subscribes a listener to a specific event type.
                 *
                 * @tparam EventType The type of the event to subscribe to.
                 * @param listener The listener to be subscribed.
                 */
                template <typename EventType> static void Subscribe(AListener &listener)
                {
                    EventId id = event_id<EventType>();
                    getInstance().m_table[id].push_back(&listener);
                }

                /**
                 * @brief Unsubscribes a listener from a specific event type.
                 *
                 * @tparam EventType The type of the event to unsubscribe from.
                 * @param listener The listener to be unsubscribed.
                 */
                template <typename EventType> static void Unsubscribe(AListener &listener)
                {
                    EventId id = event_id<EventType>();
                    auto &listeners = getInstance().m_table[id];
                    listeners.erase(std::remove(listeners.begin(), listeners.end(), &listener), listeners.end());
                }

                /**
                 * @brief Dispatches an event to all subscribed listeners.
                 *
                 * @tparam EventType The type of the event to dispatch.
                 * @param event Pointer to the event to be dispatched.
                 */
                template <typename EventType> static void Dispatch(EventType *event)
                {
                    EventId id = event->id;
                    if (getInstance().m_table.find(id) != getInstance().m_table.end()) {
                        for (auto listener : getInstance().m_table.at(id)) {
                            listener->callback(event);
                            if (event->isConsumed())
                                break;
                        }
                    }
                    delete event;
                }

            private:
                /**
                 * @brief Private constructor for singleton pattern.
                 */
                EventDispatcher() = default;
                /**
                 * @brief Destructor for EventDispatcher.
                 */
                ~EventDispatcher() = default;
                /**
                 * @brief Retrieves the singleton instance of EventDispatcher.
                 *
                 * @return Reference to the singleton EventDispatcher instance.
                 */
                static EventDispatcher &getInstance();
                /**
                 * @brief Table mapping event IDs to their subscribed listeners.
                 */
                std::unordered_map<EventId, std::vector<AListener *>> m_table;
            };
        } // namespace event
    } // namespace core
} // namespace atmo
