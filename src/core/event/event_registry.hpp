#pragma once

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include "core/registry/hierarchic_registry.hpp"

#include "events/event.hpp"

/**
 * @brief
 *
 */
#define REGISTER_EVENT(evt)                             \
    namespace                                           \
    {                                                   \
        static int _ = [] {                             \
            using namespace atmo::core::event;          \
            EventRegistry::RegisterType<events::evt>(); \
            return 0;                                   \
        }();                                            \
    }

namespace atmo::core::event
{
    /**
     * @brief Registry for events in the system. This registry allows for the registration of event types, as well as the dispatching of events to listeners.
     * Listeners can subscribe to specific event types, and can mark events as consumed to prevent further propagation.
     *
     */
    class EventRegistry : public registry::HierarchicRegistry<EventRegistry, events::Event>
    {
    public:
        /**
         * @brief
         *
         * @tparam Type
         * @return events::Event*
         */
        template <typename Type> static events::Event *Factorize()
        {
            return new Type();
        }

        /**
         * @brief Called when an event type is registered. Can be used to perform additional setup for the event type
         * related to the event.
         *
         * @tparam Type
         */
        template <typename Type> static void OnRegister()
        {
            spdlog::info("Registering event: {}", Type::FullName());
        }

        /**
         * @brief Set a callback function for a specific event type. The callback will be called whenever an event of the specified type is dispatched. The
         * callback should take a pointer to the event type as its parameter.
         *
         * @tparam EventType
         * @param callback
         */
        template <typename EventType> static void SetCallBack(std::function<void(EventType *)> callback)
        {
            auto wrapper = [callback](events::Event *baseEvent) { callback(static_cast<EventType *>(baseEvent)); };
            m_listeners[std::string(EventType::FullName())].push_back(wrapper);
        }

        /**
         * @brief Dispatch an event to all listeners subscribed to the event's type. Listeners will be called in the order they were registered. If a listener
         * marks the event as consumed, it will not be propagated to any further listeners. Note that the event will be dispatched as a pointer to the base
         * event type, so listeners should use the appropriate callback type when subscribing to events to ensure they receive the correct event type in their
         * callback.
         *
         * @tparam EventType
         * @param event
         */
        template <typename EventType> static void Dispatch(std::shared_ptr<EventType> event)
        {
            auto it = m_listeners.find(std::string(EventType::FullName()));
            if (it == m_listeners.end())
                return;

            for (auto listener : it->second) {
                listener(event.get());
                if (event->isConsumed())
                    break;
            }
        }

    private:
        /**
         * @brief Map of event type names to their registered listeners. Each listener is a callback function that takes a pointer to the base event type as its
         * parameter.
         *
         */
        static inline std::unordered_map<std::string, std::vector<std::function<void(events::Event *)>>> m_listeners;
    };
} // namespace atmo::core::event
