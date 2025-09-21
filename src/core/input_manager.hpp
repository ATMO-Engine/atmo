#pragma once

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>
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
                KeyEvent(int key) : key(key) {};

                bool pressed = false;
                bool just_pressed = false;
                bool released = false;
                int key = 0;
            };

            class MouseButtonEvent : public Event
            {
            public:
                MouseButtonEvent(int button) : button(button) {};

                bool pressed = false;
                bool released = false;
                int button = 0;
            };

        public:
            static InputManager &instance()
            {
                static InputManager instance;
                return instance;
            }

            void addEvent(const std::string &inputName, Event event);

            INTERNAL void setKeyDown(const std::string &inputName);
            bool isKeyPressed(const std::string &inputName);
            bool isKeyJustPressed(const std::string &inputName);
            INTERNAL void setKeyUp(const std::string &inputName);
            bool isKeyUp(const std::string &inputName);

            INTERNAL void setMouseButtonDown(int button);
            bool isMouseButtonPressed(int button);
            INTERNAL void setMouseButtonUp(int button);
            bool isMouseButtonUp(int button);
            INTERNAL void setMouseMotion(int x, int y);
            INTERNAL std::pair<types::vector2i, types::vector2i> getLastMouseMotion();
            INTERNAL types::vector2i getMousePosition();


        private:
            InputManager() = default;
            InputManager(const InputManager &) = delete;
            InputManager &operator=(const InputManager &) = delete;

            std::map<std::string, std::vector<std::shared_ptr<Event>>> inputs;
            std::vector<std::shared_ptr<Event>> events;
        };
    } // namespace core
} // namespace atmo
