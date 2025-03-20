#include "Window.hpp"
#include "SDL3/SDL_init.h"
#include "SDL3/SDL_render.h"
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "backends/imgui_impl_sdlrenderer3.h"

Window::Window() : shouldClose(false) {}

Window::~Window()
{
    delete textureEditor;
    delete sceneEditor;
}

bool Window::init()
{
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        spdlog::critical("Could not initialize SDL: {}\n", SDL_GetError());
        return false;
    }

    window = SDL_CreateWindow("ATMO Engine", 1920 / 1.5, 1080 / 1.5, SDL_WINDOW_OPENGL);
    if (!window) {
        spdlog::critical("Could not create window: {}\n", SDL_GetError());
        return false;
    }

    _renderer = SDL_CreateRenderer(window, nullptr);
    if (!_renderer) {
        spdlog::critical("Could not create renderer: {}\n", SDL_GetError());
        return false;
    }

#if defined(__APPLE__)
    //SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    //SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    //SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
#endif

    textureEditor = new SpriteEditor(_renderer);
    sceneEditor = new SceneEditor(_renderer);

    sceneEditor->init();
    textureEditor->init();
    return true;
}

void Window::setupImGui()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    io.IniFilename = NULL;

    ImGui_ImplSDL3_InitForSDLRenderer(window, _renderer);
    ImGui_ImplSDLRenderer3_Init(_renderer);
}

void Window::run()
{
    ImGuiIO &io = ImGui::GetIO();
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    while (!shouldClose) {
        SDL_Event event;

        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT) {
                shouldClose = true;
            }
        }

        // TODO: game logic here

        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        ImGuiViewport *viewport = ImGui::GetMainViewport();

        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, viewport->Size.y));

        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings;

        if (ImGui::Begin("TopBar", nullptr, window_flags)) {
            if (ImGui::BeginTabBar("##MainTabBar", ImGuiTabBarFlags_None)) {
                if (ImGui::BeginTabItem("Scene")) {
                    sceneEditor->run();
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Texture")) {
                    textureEditor->run();
                    ImGui::EndTabItem();
                }
                if (ImGui::TabItemButton("+")) {
                    spdlog::info("Opening new tab");
                }
                ImGui::EndTabBar();
            }
        }
        ImGui::End();

        ImGui::Render();

        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), _renderer);
        SDL_RenderPresent(_renderer);
    }
}

void Window::destroy()
{
    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    SDL_DestroyWindow(window);

    SDL_Quit();
}
