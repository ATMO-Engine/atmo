#include "input_manager.hpp"

void atmo::core::InputManager::addEvent(const std::string &inputName, Event event)
{
    if (inputs.find(inputName) == inputs.end()) {
        inputs[inputName] = {};
    }

    auto evt = std::make_shared<Event>(event);

    events.push_back(evt);
    inputs[inputName].push_back(evt);
}
