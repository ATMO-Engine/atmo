#pragma once

#include <algorithm>
#include <cstdint>
#include <typeinfo>
#include <unordered_map>
#include <vector>

#include "Ievent_listener.hpp"

namespace atmo
{
    namespace core
    {
        namespace event
        {
            using EventId = uint32_t;
            template <typename EventType> static EventId event_id()
            {
                static const EventId id = static_cast<EventId>(typeid(EventType).hash_code());
                return id;
            }


            class EventDispatcher
            {
            public:
                template <typename EventType> static void Subscribe(IListener &listener)
                {
                    EventId id = event_id<EventType>();
                    getInstance().m_table[id].push_back(&listener);
                }

                template <typename EventType> static void Unsubscribe(IListener &listener)
                {
                    EventId id = event_id<EventType>();
                    auto &listeners = getInstance().m_table[id];
                    listeners.erase(std::remove(listeners.begin(), listeners.end(), &listener), listeners.end());
                }

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
                EventDispatcher() = default;
                ~EventDispatcher() = default;
                static EventDispatcher &getInstance();
                std::unordered_map<EventId, std::vector<IListener *>> m_table;
            };
        } // namespace event
    } // namespace core
} // namespace atmo
