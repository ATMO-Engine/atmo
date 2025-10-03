#include "input_manager.hpp"
#include "spdlog/spdlog.h"

void atmo::core::InputManager::addEvent(const std::string &inputName, Event *event)
{
    bool internal = false;
    if (inputName.starts_with("#INTERNAL#"))
        internal = true;

    std::string tmp = internal ? inputName.substr(10) : inputName;

    if (inputs.find(tmp) == inputs.end())
        inputs[tmp] = {};


    auto evt = std::shared_ptr<Event>(event);

    evt->internal = internal;

    events.push_back(evt);
    inputs[tmp].push_back(evt);
}

void atmo::core::InputManager::processEvent(const SDL_Event &e, float deltaTime)
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
        case SDL_EVENT_MOUSE_WHEEL:
            for (auto evt : events)
                if (auto mouseEvent = std::dynamic_pointer_cast<MouseScrollEvent>(evt)) {
                    mouseEvent->scroll.x = e.wheel.x;
                    mouseEvent->scroll.y = e.wheel.y;
                    mouseEvent->deltaTime = deltaTime;
                }
            break;
        case SDL_EVENT_TEXT_INPUT:
            if (textInput)
                textBuffer += e.text.text;
            break;
        default:
            break;
    }
}

void atmo::core::InputManager::tick()
{
    for (auto &evt : events) {
        if (evt->getType() == Event::Type::Key) {
            auto keyEvent = std::dynamic_pointer_cast<KeyEvent>(evt);
            keyEvent->just_pressed = false;
            keyEvent->just_released = false;
        }
        if (evt->getType() == Event::Type::MouseButton) {
            auto mouseEvent = std::dynamic_pointer_cast<MouseButtonEvent>(evt);
            mouseEvent->just_pressed = false;
            mouseEvent->just_released = false;
        }
    }
}

bool atmo::core::InputManager::isPressed(const std::string &inputName)
{
    auto it = inputs.find(inputName);
    if (it == inputs.end())
        throw std::runtime_error("Input not found: " + inputName);

    for (auto evt : it->second) {
        if (evt->getType() == Event::Type::Key) {
            auto keyEvent = std::dynamic_pointer_cast<KeyEvent>(evt);
            return keyEvent->pressed;
        }
        if (evt->getType() == Event::Type::MouseButton) {
            auto keyEvent = std::dynamic_pointer_cast<MouseButtonEvent>(evt);
            return keyEvent->pressed;
        }
    }

    return false;
}

bool atmo::core::InputManager::isJustPressed(const std::string &inputName)
{
    auto it = inputs.find(inputName);
    if (it == inputs.end())
        throw std::runtime_error("Input not found: " + inputName);

    for (auto evt : it->second) {
        if (evt->getType() == Event::Type::Key) {
            auto keyEvent = std::dynamic_pointer_cast<KeyEvent>(evt);
            return keyEvent->just_pressed;
        }
        if (evt->getType() == Event::Type::MouseButton) {
            auto keyEvent = std::dynamic_pointer_cast<MouseButtonEvent>(evt);
            return keyEvent->just_pressed;
        }
    }

    return false;
}

bool atmo::core::InputManager::isJustReleased(const std::string &inputName)
{
    auto it = inputs.find(inputName);
    if (it == inputs.end())
        throw std::runtime_error("Input not found: " + inputName);

    for (auto evt : it->second) {
        if (evt->getType() == Event::Type::Key) {
            auto keyEvent = std::dynamic_pointer_cast<KeyEvent>(evt);
            return keyEvent->just_released;
        }
        if (evt->getType() == Event::Type::MouseButton) {
            auto keyEvent = std::dynamic_pointer_cast<MouseButtonEvent>(evt);
            return keyEvent->just_released;
        }
    }

    return false;
}

bool atmo::core::InputManager::isReleased(const std::string &inputName)
{
    auto it = inputs.find(inputName);
    if (it == inputs.end())
        throw std::runtime_error("Input not found: " + inputName);

    for (auto evt : it->second) {
        if (evt->getType() == Event::Type::Key) {
            auto keyEvent = std::dynamic_pointer_cast<KeyEvent>(evt);
            return keyEvent->released;
        }
        if (evt->getType() == Event::Type::MouseButton) {
            auto keyEvent = std::dynamic_pointer_cast<MouseButtonEvent>(evt);
            return keyEvent->released;
        }
    }

    return false;
}

void atmo::core::InputManager::setMousePosition(float x, float y)
{
    SDL_WarpMouseInWindow(nullptr, x, y);
}

atmo::core::types::vector2 atmo::core::InputManager::getMousePosition()
{
    atmo::core::types::vector2 pos;

    SDL_GetMouseState(&pos.x, &pos.y);
    return pos;
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
        mouseEvent->just_pressed = (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN);
        mouseEvent->released = (e.type == SDL_EVENT_MOUSE_BUTTON_UP);
        mouseEvent->just_released = (e.type == SDL_EVENT_MOUSE_BUTTON_UP);
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

std::pair<atmo::core::types::vector2, float> atmo::core::InputManager::getScrollDelta(const std::string &inputName)
{
    auto it = inputs.find(inputName);
    if (it == inputs.end())
        throw std::runtime_error("Input not found: " + inputName);

    for (auto evt : it->second) {
        if (evt->getType() == Event::Type::MouseScroll) {
            auto mouseEvent = std::dynamic_pointer_cast<MouseScrollEvent>(evt);
            return {
                {mouseEvent->scroll.x, mouseEvent->scroll.y},
                mouseEvent->deltaTime
            };
        }
    }

    return {
        {0, 0},
        0.0f
    };
}
