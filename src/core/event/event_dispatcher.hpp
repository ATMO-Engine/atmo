#pragma once

#include <functional>
#include <string_view>
#include <unordered_map>
#include <vector>
#include "Ievent.hpp"
#include "Ievent_listener.hpp"

namespace atmo
{
    namespace core
    {
        namespace event
        {
            using EventId = uint32_t;
            template <typename EventType> EventId event_id()
            {
                static const EventId id = static_cast<EventId>(typeid(EventType).hash_code());
                return id;
            }


            class EventDispatcher
            {
            public:
                EventDispatcher() = default;
                ~EventDispatcher() = default;
                template <typename EventType> void subscribe(IListener &listener)
                {
                    EventId id = event_id<EventType>();
                    m_table[id].push_back(&listener);
                }
                template <typename EventType> void unsubscribe(IListener &listener)
                {
                    EventId id = event_id<EventType>();
                    auto &listeners = m_table[id];
                    listeners.erase(std::remove(listeners.begin(), listeners.end(), &listener), listeners.end());
                }
                template <typename EventType> void dispatch(IEvent &event) const
                {
                    EventId id = event_id<EventType>();
                    if (m_table.find(id) != m_table.end()) {
                        for (auto listener : m_table.at(id)) {
                            listener->onEvent(event);
                        }
                    }
                }

            private:
                std::unordered_map<EventId, std::vector<IListener *>> m_table;
            };
        } // namespace event
    } // namespace core
} // namespace atmo
