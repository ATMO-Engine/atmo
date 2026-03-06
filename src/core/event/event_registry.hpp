#include <functional>
#include <typeindex>
#include <unordered_map>
#include <vector>
#include "SDL3/SDL_events.h"
#include "core/event/a_event_listener.hpp"
#include "core/registry/hierarchic_registry.hpp"

#include "a_event.hpp"

namespace atmo
{
    namespace core
    {
        namespace ecs
        {
            class EventRegistry : public registry::HierarchicRegistry<EventRegistry, event::AEvent>
            {
            public:
                template <typename Type> static event::AEvent *Factorize()
                {
                    return new Type();
                }

                template <typename EventType> static EventRegistry &Create()
                {
                    static EventRegistry registry;
                    registry.m_currentType = std::type_index(typeid(EventType));
                    return registry;
                }

                void setCallBack(std::function<void(event::AEvent *)> callback)
                {
                    m_listeners[m_currentType].push_back(callback);
                }

                template <typename EventType> static void Dispatch(EventType &event)
                {
                    auto it = m_listeners.find(std::type_index(typeid(EventType)));
                    if (it == m_listeners.end())
                        return;

                    for (auto &listener : it->second) {
                        listener(&event);
                        if (event.isConsumed())
                            break;
                    }
                }

            private:
                // static EventRegistry &GetInstance(); // TODO: implement this
                std::type_index m_currentType{ typeid(void) };
                static inline std::unordered_map<std::type_index, std::vector<std::function<void(event::AEvent *)>>> m_listeners;
            };
        } // namespace ecs
    } // namespace core
} // namespace atmo
