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
                Event() = default;
                virtual ~Event() = default;
            };

            class KeyEvent : public Event
            {
            public:
                KeyEvent(int key, bool scancode) : key(key), scancode(scancode) {};

                bool pressed = false;
                bool just_pressed = false;
                bool released = false;
                int key;
                bool scancode;
            };

            class MouseButtonEvent : public Event
            {
            public:
                MouseButtonEvent(int button) : button(button) {};

                bool pressed = false;
                bool released = false;
                int button;
            };

        public:
            static InputManager &instance()
            {
                static InputManager instance;
                return instance;
            }

            void addEvent(const std::string &inputName, Event event);
            void processEvent(const SDL_Event &e);

            bool isKeyPressed(const std::string &inputName);
            bool isKeyJustPressed(const std::string &inputName);
            bool isKeyUp(const std::string &inputName);

            bool isMouseButtonPressed(int button);
            bool isMouseButtonUp(int button);
            std::pair<types::vector2, types::vector2> getLastMouseMotion();
            void setMousePosition(float x, float y);
            types::vector2 getMousePosition();

            std::string consumeText() noexcept;
            void startTextInput(SDL_Window *window) noexcept;
            void stopTextInput(SDL_Window *window) noexcept;


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
