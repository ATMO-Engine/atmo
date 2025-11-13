#include "input_manager.hpp"
#include "spdlog/spdlog.h"

atmo::core::InputManager atmo::core::InputManager::instance;

void atmo::core::InputManager::AddEvent(const std::string &inputName, Event *event, bool internal)
{

    if (instance.m_inputs.find(inputName) == instance.m_inputs.end())
        instance.m_inputs[inputName] = {};


    auto evt = std::shared_ptr<Event>(event);

    evt->internal = internal;

    instance.m_events.push_back(evt);
    instance.m_inputs[inputName].push_back(evt);
}

void atmo::core::InputManager::ProcessEvent(const SDL_Event &e, float deltaTime)
{
    switch (e.type) {
        case SDL_EVENT_KEY_DOWN:
        case SDL_EVENT_KEY_UP:
            for (auto evt : instance.m_events)
                if (auto keyEvent = std::dynamic_pointer_cast<KeyEvent>(evt))
                    HandleKeyboardEvent(e.key, keyEvent);
            break;
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        case SDL_EVENT_MOUSE_BUTTON_UP:
            for (auto evt : instance.m_events)
                if (auto mouseEvent = std::dynamic_pointer_cast<MouseButtonEvent>(evt))
                    HandleMouseButtonEvent(e.button, mouseEvent);
            break;
        case SDL_EVENT_MOUSE_WHEEL:
            for (auto evt : instance.m_events)
                if (auto mouseEvent = std::dynamic_pointer_cast<MouseScrollEvent>(evt)) {
                    mouseEvent->scroll.x = e.wheel.x;
                    mouseEvent->scroll.y = e.wheel.y;
                    mouseEvent->delta_time = deltaTime;
                }
            break;
        case SDL_EVENT_TEXT_INPUT:
            if (instance.m_textInput)
                instance.m_textBuffer += e.text.text;
            break;
        default:
            break;
    }
}

void atmo::core::InputManager::Tick()
{
    for (auto &evt : instance.m_events) {
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

bool atmo::core::InputManager::IsPressed(const std::string &inputName)
{
    auto it = instance.m_inputs.find(inputName);
    if (it == instance.m_inputs.end())
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

bool atmo::core::InputManager::IsJustPressed(const std::string &inputName)
{
    auto it = instance.m_inputs.find(inputName);
    if (it == instance.m_inputs.end())
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

bool atmo::core::InputManager::IsJustReleased(const std::string &inputName)
{
    auto it = instance.m_inputs.find(inputName);
    if (it == instance.m_inputs.end())
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

bool atmo::core::InputManager::IsReleased(const std::string &inputName)
{
    auto it = instance.m_inputs.find(inputName);
    if (it == instance.m_inputs.end())
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

void atmo::core::InputManager::SetMousePosition(float x, float y)
{
    SDL_WarpMouseInWindow(nullptr, x, y);
}

atmo::core::types::vector2 atmo::core::InputManager::GetMousePosition()
{
    atmo::core::types::vector2 pos;

    SDL_GetMouseState(&pos.x, &pos.y);
    return pos;
}

void atmo::core::InputManager::HandleKeyboardEvent(const SDL_KeyboardEvent &e, std::shared_ptr<KeyEvent> keyEvent)
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

void atmo::core::InputManager::HandleMouseButtonEvent(const SDL_MouseButtonEvent &e, std::shared_ptr<MouseButtonEvent> mouseEvent)
{
    if (e.button == mouseEvent->button) {
        mouseEvent->pressed = (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN);
        mouseEvent->just_pressed = (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN);
        mouseEvent->released = (e.type == SDL_EVENT_MOUSE_BUTTON_UP);
        mouseEvent->just_released = (e.type == SDL_EVENT_MOUSE_BUTTON_UP);
    }
}


std::string atmo::core::InputManager::ConsumeText() noexcept
{
    auto t = instance.m_textBuffer;
    instance.m_textBuffer.clear();
    return t;
}

void atmo::core::InputManager::StartTextInput(SDL_Window *window) noexcept
{
    if (!instance.m_textInput) {
        SDL_StartTextInput(window);
        instance.m_textInput = true;
    }
}

void atmo::core::InputManager::StopTextInput(SDL_Window *window) noexcept
{
    if (instance.m_textInput) {
        SDL_StopTextInput(window);
        instance.m_textInput = false;
    }
}

std::pair<atmo::core::types::vector2, float> atmo::core::InputManager::GetScrollDelta(const std::string &inputName)
{
    auto it = instance.m_inputs.find(inputName);
    if (it == instance.m_inputs.end())
        throw std::runtime_error("Input not found: " + inputName);

    for (auto evt : it->second) {
        if (evt->getType() == Event::Type::MouseScroll) {
            auto mouseEvent = std::dynamic_pointer_cast<MouseScrollEvent>(evt);
            return { { mouseEvent->scroll.x, mouseEvent->scroll.y }, mouseEvent->delta_time };
        }
    }

    return { { 0, 0 }, 0.0f };
}
