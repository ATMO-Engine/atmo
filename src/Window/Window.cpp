#include "Window.hpp"
#include "SDL3/SDL_init.h"

Window::Window() : shouldClose(false) {}

Window::~Window() {}

bool Window::init()
{
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        spdlog::critical("Could not initialize SDL: {}\n", SDL_GetError());
        return false;
    }

    // const SDL_DisplayMode *mode = SDL_GetCurrentDisplayMode(0);
    // if (!mode) {
    //     spdlog::critical("Failed to get display mode: {}", SDL_GetError());
    //     return false;
    // }

    window = SDL_CreateWindow("ATMO Engine", 1920/1.5, 1080/1.5, SDL_WINDOW_OPENGL);
    if (!window) {
        spdlog::critical("Could not create window: {}\n", SDL_GetError());
        return false;
    }

#if defined(__APPLE__)
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
#endif

    context = SDL_GL_CreateContext(window);

    return true;
}

void Window::setupImGui()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    ImGui_ImplSDL3_InitForOpenGL(window, context);
    ImGui_ImplOpenGL3_Init();
}

void Window::run()
{
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    while (!shouldClose) {
        SDL_Event event;

        while (SDL_PollEvent(&event)) {
            // (Where your code calls SDL_PollEvent())
            ImGui_ImplSDL3_ProcessEvent(&event); // Forward your event to backend
            if (event.type == SDL_EVENT_QUIT) {
                shouldClose = true;
            }
        }
        // Do game logic, present a frame, etc.

        // (After event loop)
        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        // Rendering
        // (Your code clears your framebuffer, renders your other stuff etc.)
        ImGui::Render();
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w,
                     clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        // (Your code calls SDL_GL_SwapWindow() etc.)
        SDL_GL_SwapWindow(window);
    }
}

void Window::destroy()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    SDL_DestroyWindow(window);

    SDL_Quit();
}
