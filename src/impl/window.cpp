#define CLAY_IMPLEMENTATION

#include <SDL3/SDL.h>
#include <clay.h>
#include <spdlog/spdlog.h>

#include "window.hpp"

atmo::impl::WindowManager::WindowManager(atmo::core::components::Window &window) : is_main(window.main)
{
    spdlog::info("Creating window...");
    if (!SDL_CreateWindowAndRenderer(window.title.c_str(), window.size.x, window.size.y, 0, &sdl_window, &sdl_renderer))
        spdlog::error("Failed to create window: {}", SDL_GetError());

    SDL_SetWindowResizable(sdl_window, true);

    // TODO create renderer text engine with SDL_ttf

    auto totalMemSize = Clay_MinMemorySize();
    clay_arena = Clay_CreateArenaWithCapacityAndMemory(totalMemSize, SDL_malloc(totalMemSize));

    Clay_Initialize(clay_arena, {(float)window.size.x, (float)window.size.y},
                    {.errorHandlerFunction = [](Clay_ErrorData errorData)
                     { spdlog::error("Clay error: {}", errorData.errorText.chars); }});

    // TODO add sdl measure text callback here once SDL_ttf is integrated
    // Clay_SetMeasureTextFunction(SDL_MeasureText, state->rendererData.fonts);
}

atmo::impl::WindowManager::~WindowManager()
{
    if (sdl_renderer) {
        SDL_DestroyRenderer(sdl_renderer);
        sdl_renderer = nullptr;
    }

    if (sdl_window) {
        SDL_DestroyWindow(sdl_window);
        sdl_window = nullptr;
    }

    SDL_free(clay_arena.memory);

    spdlog::info("Window destroyed");
}

void atmo::impl::WindowManager::draw()
{
    // SDL_Event event;
    // while (SDL_PollEvent(&event)) {
    //     if (event.type == SDL_EVENT_QUIT) {
    //         SDL_Quit();
    //         exit(0);
    //     } else if (event.type == SDL_EVENT_WINDOW_RESIZED) {
    //         int width, height;
    //         SDL_GetWindowSize(sdl_window, &width, &height);
    //         Clay_SetViewportSize(clay_arena, {(float)width, (float)height});
    //     }
    // }

    // SDL_SetRenderDrawColor(sdl_renderer, 30, 30, 30, 255);
    // SDL_RenderClear(sdl_renderer);

    // Clay_BeginFrame(clay_arena);

    // // TODO draw stuff here

    // Clay_EndFrame(clay_arena);

    // Clay_Render(clay_arena, sdl_renderer);

    // SDL_RenderPresent(sdl_renderer);
}
