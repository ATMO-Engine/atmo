#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include "SDL3/SDL_events.h"
#include "core/types.hpp"

namespace atmo
{
    namespace core
    {
        class InputManager
        {
        private:
            class Event
            {
            public:
                virtual ~Event() = default;

                enum class Type {
                    Key,
                    MouseButton,
                    MouseScroll
                };

                virtual Type getType() const = 0;
                bool internal = false;
            };

        public:
            static InputManager &instance()
            {
                static InputManager instance;
                return instance;
            }

            void addEvent(const std::string &inputName, Event *event);
            void processEvent(const SDL_Event &e, float deltaTime);
            void tick();

            bool isPressed(const std::string &inputName);
            bool isJustPressed(const std::string &inputName);
            bool isJustReleased(const std::string &inputName);
            bool isReleased(const std::string &inputName);

            std::pair<types::vector2, types::vector2> getLastMouseMotion();
            void setMousePosition(float x, float y);
            types::vector2 getMousePosition();
            std::pair<types::vector2, float> getScrollDelta(const std::string &inputName);

            std::string consumeText() noexcept;
            void startTextInput(SDL_Window *window) noexcept;
            void stopTextInput(SDL_Window *window) noexcept;

            class KeyEvent : public Event
            {
            public:
                KeyEvent(int key, bool scancode) : key(key), scancode(scancode) {};
                Type getType() const override
                {
                    return Type::Key;
                }

                bool pressed = false;
                bool just_pressed = false;
                bool released = false;
                bool just_released = false;
                int key;
                bool scancode;
            };

            class MouseButtonEvent : public Event
            {
            public:
                MouseButtonEvent(int button) : button(button) {};
                Type getType() const override
                {
                    return Type::MouseButton;
                }

                bool pressed = false;
                bool just_pressed = false;
                bool released = false;
                bool just_released = false;
                int button;
            };

            class MouseScrollEvent : public Event
            {
            public:
                MouseScrollEvent() {};
                Type getType() const override
                {
                    return Type::MouseScroll;
                }

                types::vector2 scroll;
                float deltaTime = 0.0f;
            };

        private:
            InputManager() = default;
            InputManager(const InputManager &) = delete;
            InputManager &operator=(const InputManager &) = delete;

            void handleKeyboardEvent(const SDL_KeyboardEvent &e, std::shared_ptr<KeyEvent> keyEvent);
            void handleMouseButtonEvent(const SDL_MouseButtonEvent &e, std::shared_ptr<MouseButtonEvent> mouseEvent);

            bool textInput = false;

            std::unordered_map<std::string, std::vector<std::shared_ptr<Event>>> inputs;
            std::vector<std::shared_ptr<Event>> events;
            std::string textBuffer;
        };
    } // namespace core
} // namespace atmo
