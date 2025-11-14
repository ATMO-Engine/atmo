#pragma once

#include "core/event/Ievent_listener.hpp"
#include "spdlog/spdlog.h"


namespace atmo
{
    namespace core
    {
        namespace event
        {

            class ExempleListener : public IListener
            {
            public:
                bool called = false;
                ExempleListener();
                ~ExempleListener() override;
                void onEvent(const IEvent &event) override;
            };
        } // namespace event
    } // namespace core
} // namespace atmo
