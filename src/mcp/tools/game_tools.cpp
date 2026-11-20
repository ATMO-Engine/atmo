#include <algorithm>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "SDL3/SDL_events.h"
#include "SDL3/SDL_keyboard.h"
#include "core/engine.hpp"
#include "core/event/event_registry.hpp"
#include "core/event/events/sdl_event/input_event/input_event.hpp"
#include "core/input/input_manager.hpp"
#include "mcp/mcp_server.hpp"
#include "mcp/tools/helpers.hpp"
#include "mcp/tools/tools.hpp"

namespace atmo::mcp::tools
{
    namespace game_args
    {
        struct GameStepArgs {
            std::optional<int> frames;
            std::optional<float> dt;
        };

        struct InputEventArgs {
            std::string type;
            std::optional<std::string> key;
            std::optional<int> button;
            std::optional<float> x;
            std::optional<float> y;
            std::optional<float> dx;
            std::optional<float> dy;
            std::optional<std::string> text;
            std::optional<int> frames;
        };

        struct GameInputArgs {
            std::vector<InputEventArgs> events;
            std::optional<bool> advance;
        };

        struct GameActionArgs {
            std::string action;
            std::optional<int> frames;
            std::optional<bool> advance;
        };

        struct Step {
            std::vector<SDL_Event> events;
            std::optional<std::string> text;
            std::optional<core::types::Vector2> mouse;
            int wait = 1;
        };

        constexpr float default_step = 1.0f / 60.0f;

        glz::generic StatusJson(McpServer &server)
        {
            auto &engine = server.engine();
            const flecs::world &world = engine.getECS().getWorld();
            const ecs_world_info_t *info = world.get_info();
            glz::generic::array_t inputs;
            for (auto &name : core::InputManager::GetInputNames()) inputs.emplace_back(std::move(name));
            auto scene = engine.getECS().getCurrentScene();
            return glz::generic::object_t{
                { "frame", static_cast<double>(engine.frameIndex()) },
                { "paused", engine.isPaused() },
                { "pending_steps", static_cast<double>(engine.pendingSteps()) },
                { "time", static_cast<double>(info->world_time_total) },
                { "delta_time", static_cast<double>(info->delta_time) },
                { "scene", scene ? std::string(scene->name()) : std::string() },
                { "inputs", std::move(inputs) },
            };
        }

        std::optional<SDL_Keycode> ParseKey(const std::string &name)
        {
            SDL_Keycode key = SDL_GetKeyFromName(name.c_str());
            if (key != SDLK_UNKNOWN)
                return key;
            SDL_Scancode scancode = SDL_GetScancodeFromName(name.c_str());
            if (scancode != SDL_SCANCODE_UNKNOWN)
                return SDL_GetKeyFromScancode(scancode, SDL_KMOD_NONE, false);
            return std::nullopt;
        }

        SDL_Event KeyEvent(SDL_Keycode key, bool down)
        {
            SDL_Event event{};
            event.type = down ? SDL_EVENT_KEY_DOWN : SDL_EVENT_KEY_UP;
            event.key.key = key;
            event.key.scancode = SDL_GetScancodeFromKey(key, nullptr);
            event.key.down = down;
            return event;
        }

        SDL_Event ButtonEvent(int button, bool down, std::optional<float> x, std::optional<float> y)
        {
            SDL_Event event{};
            event.type = down ? SDL_EVENT_MOUSE_BUTTON_DOWN : SDL_EVENT_MOUSE_BUTTON_UP;
            event.button.button = static_cast<Uint8>(button);
            event.button.down = down;
            event.button.clicks = 1;
            event.button.x = x.value_or(0.0f);
            event.button.y = y.value_or(0.0f);
            return event;
        }

        std::expected<std::vector<Step>, std::string> BuildSteps(const std::vector<InputEventArgs> &events)
        {
            std::vector<Step> steps;
            for (const auto &input : events) {
                const int wait = std::max(0, input.frames.value_or(1));
                std::optional<core::types::Vector2> mouse;
                if (input.x && input.y)
                    mouse = core::types::Vector2{ *input.x, *input.y };

                if (input.type == "key_down" || input.type == "key_up" || input.type == "key_press") {
                    if (!input.key)
                        return std::unexpected(input.type + " needs 'key'");
                    auto key = ParseKey(*input.key);
                    if (!key)
                        return std::unexpected("Unknown key name: " + *input.key + " (SDL key names: 'A', 'Space', 'Return', 'Left', 'Escape'...)");
                    if (input.type == "key_press") {
                        steps.push_back({ { KeyEvent(*key, true) }, std::nullopt, std::nullopt, std::max(1, wait) });
                        steps.push_back({ { KeyEvent(*key, false) }, std::nullopt, std::nullopt, 1 });
                    } else {
                        steps.push_back({ { KeyEvent(*key, input.type == "key_down") }, std::nullopt, std::nullopt, wait });
                    }
                } else if (input.type == "mouse_down" || input.type == "mouse_up" || input.type == "mouse_click") {
                    const int button = input.button.value_or(SDL_BUTTON_LEFT);
                    if (input.type == "mouse_click") {
                        steps.push_back({ { ButtonEvent(button, true, input.x, input.y) }, std::nullopt, mouse, std::max(1, wait) });
                        steps.push_back({ { ButtonEvent(button, false, input.x, input.y) }, std::nullopt, mouse, 1 });
                    } else {
                        steps.push_back({ { ButtonEvent(button, input.type == "mouse_down", input.x, input.y) }, std::nullopt, mouse, wait });
                    }
                } else if (input.type == "mouse_move") {
                    if (!mouse)
                        return std::unexpected("mouse_move needs 'x' and 'y'");
                    SDL_Event event{};
                    event.type = SDL_EVENT_MOUSE_MOTION;
                    event.motion.x = mouse->x;
                    event.motion.y = mouse->y;
                    steps.push_back({ { event }, std::nullopt, mouse, wait });
                } else if (input.type == "wheel") {
                    SDL_Event event{};
                    event.type = SDL_EVENT_MOUSE_WHEEL;
                    event.wheel.x = input.dx.value_or(0.0f);
                    event.wheel.y = input.dy.value_or(0.0f);
                    steps.push_back({ { event }, std::nullopt, mouse, wait });
                } else if (input.type == "text") {
                    if (!input.text)
                        return std::unexpected("text needs 'text'");
                    steps.push_back({ {}, input.text, std::nullopt, wait });
                } else {
                    return std::unexpected("Unknown event type: " + input.type);
                }
            }
            return steps;
        }

        void Apply(const Step &step)
        {
            if (step.mouse)
                core::InputManager::SetMousePosition(step.mouse->x, step.mouse->y);
            for (SDL_Event event : step.events) SDL_PushEvent(&event);
            if (step.text) {
                SDL_Event event{};
                event.type = SDL_EVENT_TEXT_INPUT;
                event.text.text = step.text->c_str();
                auto input_event = core::event::EventRegistry::Create<core::event::events::InputEvent>("Event::SDLEvent::InputEvent");
                input_event->sdl_event = event;
                core::event::EventRegistry::Dispatch(input_event);
            }
        }

        void Run(McpServer &server, std::shared_ptr<std::vector<Step>> steps, std::size_t index, Reply reply)
        {
            while (index < steps->size()) {
                const Step &step = (*steps)[index++];
                Apply(step);
                const int wait = index == steps->size() ? std::max(1, step.wait) : step.wait;
                if (wait > 0) {
                    server.after(static_cast<std::uint64_t>(wait), [&server, steps, index, reply] { Run(server, steps, index, reply); });
                    return;
                }
            }
            reply.json(StatusJson(server));
        }

        int TotalFrames(const std::vector<Step> &steps)
        {
            int total = 0;
            for (const auto &step : steps) total += step.wait;
            if (!steps.empty() && steps.back().wait == 0)
                ++total;
            return total;
        }
    } // namespace game_args
} // namespace atmo::mcp::tools

using namespace atmo::mcp::tools::game_args;

template <> struct glz::json_schema<GameStepArgs> {
    schema frames{ .description = "Number of frames to run (default 1)" };
    schema dt{ .description = "Delta time of each frame in seconds (default 1/60)" };
};
template <> struct glz::json_schema<InputEventArgs> {
    schema type{ .description = "key_down, key_up, key_press, mouse_down, mouse_up, mouse_click, mouse_move, wheel or text" };
    schema key{ .description = "SDL key name: 'A', 'Space', 'Return', 'Left', 'Escape'..." };
    schema button{ .description = "Mouse button: 1 left (default), 2 middle, 3 right" };
    schema x{ .description = "Mouse x in window coordinates" };
    schema y{ .description = "Mouse y in window coordinates" };
    schema dx{ .description = "Horizontal wheel amount" };
    schema dy{ .description = "Vertical wheel amount" };
    schema text{ .description = "Text typed (type 'text')" };
    schema frames{ .description = "Frames to wait after this event (default 1); for key_press and mouse_click, how long the key is held" };
};
template <> struct glz::json_schema<GameInputArgs> {
    schema events{ .description = "Events applied in order, one or more frames apart" };
    schema advance{ .description = "When the game is paused, step it through the frames the events need (default true)" };
};
template <> struct glz::json_schema<GameActionArgs> {
    schema action{ .description = "Input name bound with InputManager (see 'inputs' in game_status)" };
    schema frames{ .description = "Frames to hold the action (default 1)" };
    schema advance{ .description = "When the game is paused, step it through those frames (default true)" };
};

namespace atmo::mcp::tools
{
    void RegisterGameTools(McpServer &server)
    {
        auto &tools = server.tools();

        tools.add<NoArgs>("game_status", "Frame, time, pause state and input names of the running game.", [&server](const NoArgs &, const Reply &reply) {
            reply.json(StatusJson(server));
        });

        tools.add<NoArgs>("game_pause", "Pause the game: systems keep running with a delta time of 0.", [&server](const NoArgs &, const Reply &reply) {
            server.engine().setPaused(true);
            reply.json(StatusJson(server));
        });

        tools.add<NoArgs>("game_resume", "Resume the game in real time.", [&server](const NoArgs &, const Reply &reply) {
            server.engine().setPaused(false);
            reply.json(StatusJson(server));
        });

        tools.add<GameStepArgs>(
            "game_step",
            "Pause the game and advance it by a number of frames with a fixed delta time; returns once they have run.",
            [&server](const GameStepArgs &args, const Reply &reply) {
                const int frames = std::clamp(args.frames.value_or(1), 1, 100000);
                server.engine().step(frames, std::max(0.0f, args.dt.value_or(default_step)));
                server.after(static_cast<std::uint64_t>(frames), [&server, reply] { reply.json(StatusJson(server)); });
            });

        tools.add<GameInputArgs>(
            "game_input",
            "Send keyboard, mouse and text input to the game, as if the window received it; returns once the events were processed.",
            [&server](const GameInputArgs &args, const Reply &reply) {
                auto steps = BuildSteps(args.events);
                if (!steps)
                    return reply.error(steps.error());
                if (server.engine().isPaused() && args.advance.value_or(true))
                    server.engine().step(TotalFrames(*steps), default_step);
                Run(server, std::make_shared<std::vector<Step>>(std::move(*steps)), 0, reply);
            });

        tools.add<GameActionArgs>(
            "game_action", "Press a named input binding for some frames, then release it.", [&server](const GameActionArgs &args, const Reply &reply) {
                auto down = core::InputManager::MakeEventsForInput(args.action, true);
                if (down.empty())
                    return reply.error("No key or mouse binding named '" + args.action + "' (see 'inputs' in game_status)");
                const int frames = std::max(1, args.frames.value_or(1));
                auto steps = std::make_shared<std::vector<Step>>();
                steps->push_back({ down, std::nullopt, std::nullopt, frames });
                steps->push_back({ core::InputManager::MakeEventsForInput(args.action, false), std::nullopt, std::nullopt, 1 });
                if (server.engine().isPaused() && args.advance.value_or(true))
                    server.engine().step(TotalFrames(*steps), default_step);
                Run(server, steps, 0, reply);
            });
    }
} // namespace atmo::mcp::tools
