#include <SDL3/SDL.h>
#include <spdlog/spdlog.h>
#include "core/input_manager.hpp"

#include "window.hpp"

#define CLAY_IMPLEMENTATION
#include <clay.h>

void SDL_Clay_RenderClayCommands(Clay_SDL3RendererData *rendererData, Clay_RenderCommandArray *rcommands);

atmo::impl::WindowManager::WindowManager(const atmo::core::components::Window &window) : is_main(window.main)
{
    if (!SDL_CreateWindowAndRenderer(
            window.title.c_str(), window.size.x, window.size.y, 0, &this->window, &rendererData.renderer
        ))
        spdlog::error("Failed to create window: {}", SDL_GetError());

    SDL_SetWindowResizable(this->window, true);

    // TODO create renderer text engine with SDL_ttf

    auto totalMemSize = Clay_MinMemorySize();
    clay_arena = Clay_CreateArenaWithCapacityAndMemory(totalMemSize, SDL_malloc(totalMemSize));

    Clay_Initialize(
        clay_arena, {(float)window.size.x, (float)window.size.y},
        {.errorHandlerFunction = [](Clay_ErrorData errorData) {
            spdlog::error("Clay error: {}", errorData.errorText.chars);
        }}
    );

    // TODO add sdl measure text callback here once SDL_ttf is integrated
    // Clay_SetMeasureTextFunction(SDL_MeasureText, state->rendererData.fonts);
}

atmo::impl::WindowManager::~WindowManager()
{
    if (rendererData.renderer) {
        SDL_DestroyRenderer(rendererData.renderer);
        rendererData.renderer = nullptr;
    }

    if (window) {
        SDL_DestroyWindow(window);
        window = nullptr;
    }

    SDL_free(clay_arena.memory);
}

void atmo::impl::WindowManager::pollEvents(float deltaTime)
{
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_EVENT_WINDOW_RESIZED:
                Clay_SetLayoutDimensions({(float)event.window.data1, (float)event.window.data2});
                break;
            // case SDL_EVENT_MOUSE_MOTION:
            //     Clay_SetPointerState({event.motion.x, event.motion.y}, event.motion.state & SDL_BUTTON_LMASK);
            //     break;
            // case SDL_EVENT_MOUSE_BUTTON_DOWN:
            //     Clay_SetPointerState({event.button.x, event.button.y}, event.button.button == SDL_BUTTON_LEFT);
            //     break;
            // case SDL_EVENT_MOUSE_WHEEL:
            //     Clay_UpdateScrollContainers(true, {event.wheel.x, event.wheel.y}, deltaTime);
            //     break;
            default:
                core::InputManager::instance().processEvent(event);
                break;
        };
    }
}

void atmo::impl::WindowManager::draw()
{
    spdlog::debug("Drawing window...");
    Clay_BeginLayout();

    // TODO build layout from entities in ecs here (only children of this window)

    auto commands = Clay_EndLayout();

    SDL_SetRenderDrawColor(rendererData.renderer, 0, 0, 0, 255);
    SDL_RenderClear(rendererData.renderer);

    SDL_Clay_RenderClayCommands(&rendererData, &commands);

    SDL_RenderPresent(rendererData.renderer);
}

void atmo::impl::WindowManager::rename(const std::string &name) noexcept
{
    if (window)
        SDL_SetWindowTitle(window, name.c_str());
}

void atmo::impl::WindowManager::resize(const atmo::core::types::vector2i &size) noexcept
{
    if (window)
        SDL_SetWindowSize(window, size.x, size.y);
}

atmo::core::types::vector2i atmo::impl::WindowManager::getSize() const noexcept
{
    if (window) {
        int w, h;
        SDL_GetWindowSize(window, &w, &h);
        return {w, h};
    }
    return {-1, -1};
}

std::string atmo::impl::WindowManager::getTitle() const noexcept
{
    if (window)
        return std::string(SDL_GetWindowTitle(window));
    return "";
}
