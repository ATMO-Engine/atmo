#include "input_manager.hpp"
#include <iostream>
#include "SDL3/SDL_events.h"
#include "core/event/events/sdl_event/input_event/input_event.hpp"
#include "spdlog/spdlog.h"

#include "impl/profiler.hpp"

atmo::core::InputManager::InputManager()
{
    event::EventRegistry::SetCallBack<event::events::InputEvent>([](event::events::InputEvent *event) { ProcessEvent(event->sdl_event, event->delta); });
};


void atmo::core::InputManager::AddInput(const std::string &inputName, Input *event, bool internal)
{
    if (Instance().p_inputs.find(inputName) == Instance().p_inputs.end())
        Instance().p_inputs[inputName] = {};

    auto evt = std::shared_ptr<Input>(event);

    evt->internal = internal;

    Instance().p_events.push_back(evt);
    Instance().p_inputs[inputName].push_back(evt);
}

void atmo::core::InputManager::ProcessEvent(const SDL_Event &e, float deltaTime)
{
    switch (e.type) {
        case SDL_EVENT_KEY_DOWN:
        case SDL_EVENT_KEY_UP:
            if (e.type == SDL_EVENT_KEY_DOWN && Instance().p_textInput)
                Instance().p_textEvents.emplace_back(TextKeyDown{ e.key.key, e.key.scancode, e.key.mod, e.key.repeat });
            for (auto evt : Instance().p_events)
                if (auto keyEvent = std::dynamic_pointer_cast<KeyEvent>(evt))
                    HandleKeyboardEvent(e.key, keyEvent);
            break;
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        case SDL_EVENT_MOUSE_BUTTON_UP:
            if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN && e.button.button == SDL_BUTTON_LEFT)
                Instance().p_lastClickCount = e.button.clicks;
            for (auto evt : Instance().p_events)
                if (auto mouseEvent = std::dynamic_pointer_cast<MouseButtonEvent>(evt))
                    HandleMouseButtonEvent(e.button, mouseEvent);
            break;
        case SDL_EVENT_MOUSE_WHEEL:
            for (auto evt : Instance().p_events)
                if (auto mouseEvent = std::dynamic_pointer_cast<MouseScrollEvent>(evt)) {
                    mouseEvent->scroll.x = e.wheel.x;
                    mouseEvent->scroll.y = e.wheel.y;
                    mouseEvent->delta_time = deltaTime;
                }
            break;
        case SDL_EVENT_TEXT_INPUT:
            if (Instance().p_textInput)
                Instance().p_textEvents.emplace_back(TextCommit{ e.text.text });
            break;
        case SDL_EVENT_TEXT_EDITING:
            if (Instance().p_textInput)
                Instance().p_textEvents.emplace_back(TextComposition{ e.edit.text ? e.edit.text : "", e.edit.start, e.edit.length });
            break;
        case SDL_EVENT_PINCH_UPDATE:
            for (auto evt : Instance().p_events)
                if (auto pinchEvent = std::dynamic_pointer_cast<PinchEvent>(evt)) {
                    pinchEvent->scale = e.pinch.scale;
                }
            break;
        default:
            break;
    }
}

void atmo::core::InputManager::Tick()
{
    ATMO_PROFILE_SCOPE_COLOR(0xFFFFFF);
    for (auto &evt : Instance().p_events) {
        switch (evt->getType()) {
            case Input::Type::Key:
                {
                    auto keyEvent = std::dynamic_pointer_cast<KeyEvent>(evt);
                    keyEvent->just_pressed = false;
                    keyEvent->just_released = false;
                }
                break;
            case Input::Type::MouseButton:
                {
                    auto mouseButtonEvent = std::dynamic_pointer_cast<MouseButtonEvent>(evt);
                    mouseButtonEvent->just_pressed = false;
                    mouseButtonEvent->just_released = false;
                }
                break;
            case Input::Type::MouseScroll:
                {
                    auto mouseScrollEvent = std::dynamic_pointer_cast<MouseScrollEvent>(evt);
                    mouseScrollEvent->scroll = { 0.0f, 0.0f };
                    mouseScrollEvent->delta_time = 0.0f;
                }
                break;
            case Input::Type::FingerPinch:
                {
                    auto pinchEvent = std::dynamic_pointer_cast<PinchEvent>(evt);
                    pinchEvent->scale = 0.0f;
                }
                break;
        }
        if (evt->getType() == Input::Type::MouseScroll) {
            auto mouseEvent = std::dynamic_pointer_cast<MouseScrollEvent>(evt);
            mouseEvent->scroll = { 0.0f, 0.0f };
        }
    }
}

bool atmo::core::InputManager::IsPressed(const std::string &inputName)
{
    auto it = Instance().p_inputs.find(inputName);
    if (it == Instance().p_inputs.end())
        throw std::runtime_error("Input not found: " + inputName);

    for (auto evt : it->second) {
        if (evt->getType() == Input::Type::Key) {
            auto keyEvent = std::dynamic_pointer_cast<KeyEvent>(evt);
            return keyEvent->pressed;
        }
        if (evt->getType() == Input::Type::MouseButton) {
            auto keyEvent = std::dynamic_pointer_cast<MouseButtonEvent>(evt);
            return keyEvent->pressed;
        }
    }

    return false;
}

bool atmo::core::InputManager::IsJustPressed(const std::string &inputName)
{
    auto it = Instance().p_inputs.find(inputName);
    if (it == Instance().p_inputs.end())
        throw std::runtime_error("Input not found: " + inputName);

    for (auto evt : it->second) {
        if (evt->getType() == Input::Type::Key) {
            auto keyEvent = std::dynamic_pointer_cast<KeyEvent>(evt);
            return keyEvent->just_pressed;
        }
        if (evt->getType() == Input::Type::MouseButton) {
            auto keyEvent = std::dynamic_pointer_cast<MouseButtonEvent>(evt);
            return keyEvent->just_pressed;
        }
    }

    return false;
}

bool atmo::core::InputManager::IsJustReleased(const std::string &inputName)
{
    auto it = Instance().p_inputs.find(inputName);
    if (it == Instance().p_inputs.end())
        throw std::runtime_error("Input not found: " + inputName);

    for (auto evt : it->second) {
        if (evt->getType() == Input::Type::Key) {
            auto keyEvent = std::dynamic_pointer_cast<KeyEvent>(evt);
            return keyEvent->just_released;
        }
        if (evt->getType() == Input::Type::MouseButton) {
            auto keyEvent = std::dynamic_pointer_cast<MouseButtonEvent>(evt);
            return keyEvent->just_released;
        }
    }

    return false;
}

bool atmo::core::InputManager::IsReleased(const std::string &inputName)
{
    auto it = Instance().p_inputs.find(inputName);
    if (it == Instance().p_inputs.end())
        throw std::runtime_error("Input not found: " + inputName);

    for (auto evt : it->second) {
        if (evt->getType() == Input::Type::Key) {
            auto keyEvent = std::dynamic_pointer_cast<KeyEvent>(evt);
            return keyEvent->released;
        }
        if (evt->getType() == Input::Type::MouseButton) {
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

atmo::core::types::Vector2 atmo::core::InputManager::GetMousePosition()
{
    atmo::core::types::Vector2 pos;

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


std::vector<atmo::core::TextInputEvent> atmo::core::InputManager::ConsumeTextEvents() noexcept
{
    return std::exchange(Instance().p_textEvents, {});
}

void atmo::core::InputManager::StartTextInput(SDL_Window *window) noexcept
{
    if (!Instance().p_textInput) {
        SDL_StartTextInput(window);
        Instance().p_textInput = true;
    }
}

void atmo::core::InputManager::StopTextInput(SDL_Window *window) noexcept
{
    if (Instance().p_textInput) {
        SDL_StopTextInput(window);
        Instance().p_textInput = false;
        Instance().p_textEvents.clear();
    }
}

bool atmo::core::InputManager::IsTextInputActive() noexcept
{
    return Instance().p_textInput;
}

void atmo::core::InputManager::SetTextInputArea(SDL_Window *window, const SDL_Rect &area, int cursor) noexcept
{
    if (window)
        SDL_SetTextInputArea(window, &area, cursor);
}

int atmo::core::InputManager::GetLastClickCount() noexcept
{
    return Instance().p_lastClickCount;
}

std::pair<atmo::core::types::Vector2, float> atmo::core::InputManager::GetScrollDelta(const std::string &inputName)
{
    auto it = Instance().p_inputs.find(inputName);
    if (it == Instance().p_inputs.end())
        throw std::runtime_error("Input not found: " + inputName);

    for (auto evt : it->second) {
        if (evt->getType() == Input::Type::MouseScroll) {
            auto mouseEvent = std::dynamic_pointer_cast<MouseScrollEvent>(evt);
            return { { mouseEvent->scroll.x, mouseEvent->scroll.y }, mouseEvent->delta_time };
        }
    }

    return { { 0, 0 }, 0.0f };
}

float atmo::core::InputManager::GetPinchScale(const std::string &inputName)
{
    auto it = Instance().p_inputs.find(inputName);
    if (it == Instance().p_inputs.end())
        throw std::runtime_error("Input not found: " + inputName);

    for (auto evt : it->second) {
        if (evt->getType() == Input::Type::FingerPinch) {
            auto pinchEvent = std::dynamic_pointer_cast<PinchEvent>(evt);
            return pinchEvent->scale;
        }
    }

    return 0.0f;
}
