#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include "SDL3/SDL_events.h"
#include "core/types.hpp"


namespace atmo::core
{
    class InputManager
    {
    public:
    private:
        class Input
        {
        public:
            virtual ~Input() = default;

            enum class Type {
                Key,
                MouseButton,
                MouseScroll
            };

            virtual Type getType() const = 0;
            bool internal = false;
        };


    public:
        static void AddInput(const std::string &inputName, Input *event, bool internal = false);
        static void ProcessEvent(const SDL_Event &e, float deltaTime);
        static void Tick();

        static bool IsPressed(const std::string &inputName);
        static bool IsJustPressed(const std::string &inputName);
        static bool IsJustReleased(const std::string &inputName);
        static bool IsReleased(const std::string &inputName);

        static std::pair<types::Vector2, types::Vector2> GetLastMouseMotion();
        static void SetMousePosition(float x, float y);
        static types::Vector2 GetMousePosition();
        static std::pair<types::Vector2, float> GetScrollDelta(const std::string &inputName);

        static std::string ConsumeText() noexcept;
        static void StartTextInput(SDL_Window *window) noexcept;
        static void StopTextInput(SDL_Window *window) noexcept;

        static InputManager &Instance()
        {
            static InputManager instance;
            return instance;
        }

        class KeyEvent : public Input
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

        class MouseButtonEvent : public Input
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

        class MouseScrollEvent : public Input
        {
        public:
            MouseScrollEvent() {};
            Type getType() const override
            {
                return Type::MouseScroll;
            }

            types::Vector2 scroll;
            float delta_time = 0.0f;
        };

    protected:
        InputManager();

        static void HandleKeyboardEvent(const SDL_KeyboardEvent &e, std::shared_ptr<KeyEvent> keyEvent);
        static void HandleMouseButtonEvent(const SDL_MouseButtonEvent &e, std::shared_ptr<MouseButtonEvent> mouseEvent);

        bool p_textInput = false;

        std::unordered_map<std::string, std::vector<std::shared_ptr<Input>>> p_inputs;
        std::vector<std::shared_ptr<Input>> p_events;
        std::string p_textBuffer;
    };
} // namespace atmo::core
