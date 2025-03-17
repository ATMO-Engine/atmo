#include "Window.hpp"
#include <glad/glad.h>
#include "SDL3/SDL_init.h"
#include "imgui.h"
#include "imgui_internal.h"

Window::Window() : shouldClose(false) {}

Window::~Window() {
    delete textureEditor;
    delete sceneEditor;
}

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

    window = SDL_CreateWindow("ATMO Engine", 1920 / 1.5, 1080 / 1.5, SDL_WINDOW_OPENGL);
    if (!window) {
        spdlog::critical("Could not create window: {}\n", SDL_GetError());
        return false;
    }


    SDL_GLContext glContext = SDL_GL_CreateContext(window);
    if (!glContext) {
        return false;
    }

        // Initialize GLAD (make sure it's done after the OpenGL context is created)
        if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
            return false;
        }

#if defined(__APPLE__)
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
#endif

    context = SDL_GL_CreateContext(window);

    textureEditor = new SpriteEditor(window);
    sceneEditor = new SceneEditor();

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
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    io.IniFilename = NULL;

    ImGui_ImplSDL3_InitForOpenGL(window, context);
    ImGui_ImplOpenGL3_Init();
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

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        ImGuiViewport *viewport = ImGui::GetMainViewport();

        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, viewport->Size.y));
        ImGui::SetNextWindowViewport(viewport->ID);

        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoDocking;

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
                    spdlog::info("pressed");
                    // TODO: modal to create new tab choosing from list of available tabs
                }
                ImGui::EndTabBar();
            }
        }
        ImGui::End();

        // ImGui::DockSpaceOverViewport(viewport->ID);

        ImGui::Render();
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w,
                     clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
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
