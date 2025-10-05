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

void atmo::core::InputManager::processEvent(const SDL_Event &e)
{
    switch (e.type) {
        case SDL_EVENT_KEY_DOWN:
        case SDL_EVENT_KEY_UP:
            for (auto evt : events)
                if (auto keyEvent = std::dynamic_pointer_cast<KeyEvent>(evt))
                    handleKeyboardEvent(e.key, keyEvent);
            break;
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        case SDL_EVENT_MOUSE_BUTTON_UP:
            for (auto evt : events)
                if (auto mouseEvent = std::dynamic_pointer_cast<MouseButtonEvent>(evt))
                    handleMouseButtonEvent(e.button, mouseEvent);
            break;
        case SDL_EVENT_TEXT_INPUT:
            if (textInput)
                textBuffer += e.text.text;
            break;
        default:
            break;
    }
}

bool atmo::core::InputManager::isKeyPressed(const std::string &inputName)
{
    if (inputs.find(inputName) == inputs.end())
        return false;

    for (auto evt : inputs[inputName])
        if (auto keyEvent = std::dynamic_pointer_cast<KeyEvent>(evt))
            if (keyEvent->pressed)
                return true;

    return false;
}

bool atmo::core::InputManager::isKeyJustPressed(const std::string &inputName)
{
    if (inputs.find(inputName) == inputs.end())
        return false;

    for (auto evt : inputs[inputName])
        if (auto keyEvent = std::dynamic_pointer_cast<KeyEvent>(evt))
            if (keyEvent->just_pressed)
                return true;

    return false;
}

bool atmo::core::InputManager::isKeyUp(const std::string &inputName)
{
    if (inputs.find(inputName) == inputs.end())
        return false;

    for (auto evt : inputs[inputName])
        if (auto keyEvent = std::dynamic_pointer_cast<KeyEvent>(evt))
            if (keyEvent->released)
                return true;

    return false;
}

bool atmo::core::InputManager::isMouseButtonPressed(int button)
{
    for (auto evt : events)
        if (auto mouseEvent = std::dynamic_pointer_cast<MouseButtonEvent>(evt))
            if (mouseEvent->button == button && mouseEvent->pressed)
                return true;

    return false;
}

bool atmo::core::InputManager::isMouseButtonUp(int button)
{
    for (auto evt : events)
        if (auto mouseEvent = std::dynamic_pointer_cast<MouseButtonEvent>(evt))
            if (mouseEvent->button == button && mouseEvent->released)
                return true;

    return false;
}

void atmo::core::InputManager::handleKeyboardEvent(const SDL_KeyboardEvent &e, std::shared_ptr<KeyEvent> keyEvent)
{
    if (keyEvent->scancode && e.scancode == keyEvent->key) {
        keyEvent->pressed = (e.type == SDL_EVENT_KEY_DOWN);
        keyEvent->just_pressed = (e.type == SDL_EVENT_KEY_DOWN);
        keyEvent->released = (e.type == SDL_EVENT_KEY_UP);
    } else if (!keyEvent->scancode && e.key == keyEvent->key) {
        keyEvent->pressed = (e.type == SDL_EVENT_KEY_DOWN);
        keyEvent->just_pressed = (e.type == SDL_EVENT_KEY_DOWN);
        keyEvent->released = (e.type == SDL_EVENT_KEY_UP);
    }
}

void atmo::core::InputManager::handleMouseButtonEvent(
    const SDL_MouseButtonEvent &e, std::shared_ptr<MouseButtonEvent> mouseEvent
)
{
    if (e.button == mouseEvent->button) {
        mouseEvent->pressed = (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN);
        mouseEvent->released = (e.type == SDL_EVENT_MOUSE_BUTTON_UP);
    }
}


std::string atmo::core::InputManager::consumeText() noexcept
{
    auto t = textBuffer;
    textBuffer.clear();
    return t;
}

void atmo::core::InputManager::startTextInput(SDL_Window *window) noexcept
{
    if (!textInput) {
        SDL_StartTextInput(window);
        textInput = true;
    }
}

void atmo::core::InputManager::stopTextInput(SDL_Window *window) noexcept
{
    if (textInput) {
        SDL_StopTextInput(window);
        textInput = false;
    }
}
