#pragma once

#include <cstdint>
#include <flecs.h>


namespace atmo
{
    namespace core
    {
        namespace event
        {
            using EventId = uint32_t;

            /**
             * @brief Abstract base class for events.
             */
            class AEvent
            {
            public:
                /**
                 * @brief Virtual destructor for AEvent.
                 */
                virtual ~AEvent() = default;
                /**
                 * @brief Checks if the event has been consumed.
                 *
                 * @return true if the event is consumed, false otherwise.
                 */
                bool isConsumed() const;
                /**
                 * @brief Marks the event as consumed.
                 */
                void consume();

                /**
                 * @brief Unique identifier for the event type.
                 */
                EventId id;

            private:
                /**
                 * @brief Flag indicating whether the event has been consumed.
                 */
                bool m_consumed = false;
            };

            class UIEvent : public AEvent
            {
            public:
                UIEvent() = default;
                ~UIEvent() override = default;

            private:
                flecs::entity m_entity_id;
            };

            class HoverEvent : public UIEvent
            {
            public:
                HoverEvent() = default;
                ~HoverEvent() override = default;
            };
        } // namespace event
    } // namespace core
} // namespace atmo
