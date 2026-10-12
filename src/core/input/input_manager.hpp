#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>
#include "SDL3/SDL_events.h"
#include "core/types.hpp"


namespace atmo::core
{
    /// Key press delivered to a focused text input, in order and including key repeats.
    struct TextKeyDown {
        SDL_Keycode key = SDLK_UNKNOWN;
        SDL_Scancode scancode = SDL_SCANCODE_UNKNOWN;
        SDL_Keymod mod = SDL_KMOD_NONE;
        bool repeat = false;
    };

    /// Committed text (typed character, IME commit).
    struct TextCommit {
        std::string text;
    };

    /// In-progress IME composition. An empty `text` ends the composition.
    struct TextComposition {
        std::string text;
        int start = 0;
        int length = 0;
    };

    using TextInputEvent = std::variant<TextKeyDown, TextCommit, TextComposition>;

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
                MouseScroll,
                FingerPinch
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
        static float GetPinchScale(const std::string &inputName);

        /// Returns and clears the ordered keyboard/text events received while text input was active.
        static std::vector<TextInputEvent> ConsumeTextEvents() noexcept;
        static void StartTextInput(SDL_Window *window) noexcept;
        static void StopTextInput(SDL_Window *window) noexcept;
        static bool IsTextInputActive() noexcept;
        /// Tells the IME where the caret is so that its candidate window is placed next to it.
        static void SetTextInputArea(SDL_Window *window, const SDL_Rect &area, int cursor) noexcept;
        /// Click count (1 = single, 2 = double...) of the last left mouse button press.
        static int GetLastClickCount() noexcept;

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

        class PinchEvent : public Input
        {
        public:
            PinchEvent() {};
            Type getType() const override
            {
                return Type::FingerPinch;
            }

            float scale = 0.0f;
        };

    protected:
        InputManager();

        static void HandleKeyboardEvent(const SDL_KeyboardEvent &e, std::shared_ptr<KeyEvent> keyEvent);
        static void HandleMouseButtonEvent(const SDL_MouseButtonEvent &e, std::shared_ptr<MouseButtonEvent> mouseEvent);

        bool p_textInput = false;
        int p_lastClickCount = 1;

        std::unordered_map<std::string, std::vector<std::shared_ptr<Input>>> p_inputs;
        std::vector<std::shared_ptr<Input>> p_events;
        std::vector<TextInputEvent> p_textEvents;
    };
} // namespace atmo::core
