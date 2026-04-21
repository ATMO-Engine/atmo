#include "SDL3/SDL_video.h"
#include "SDL3_ttf/SDL_ttf.h"

#include "core/ecs/components.hpp"
#include "core/ecs/entity_registry.hpp"
#include "core/event/events/sdl_event/input_event/input_event.hpp"
#include "core/input/input_manager.hpp"
#include "core/resource/resource_manager.hpp"
#include "impl/clay_types.hpp"
#include "meta/auto_register.hpp"
#include "spdlog/spdlog.h"
#include "window.hpp"

#include "core/event/event_registry.hpp"

#define CLAY_IMPLEMENTATION
#include <clay.h>

void SDL_Clay_RenderClayCommands(Clay_SDL3RendererData *rendererData, Clay_RenderCommandArray *rcommands);

namespace atmo::core::ecs::entities
{
    void Window::RegisterComponents(flecs::world *world)
    {
        world->component<components::Window>();
    }

    void Window::RegisterSystems(flecs::world *world)
    {
        world->system<components::Window>("PollEvents").kind(flecs::PreUpdate).each([](flecs::iter &it, size_t i, components::Window &window) {
            entities::Window entity(it.entity(i));
            entity.pollEvents(it.delta_time());
            entity.beginDraw(window);
        });

        world->system<components::Window>("Draw").kind(flecs::PostUpdate).each([](flecs::iter &it, size_t i, components::Window &window) {
            entities::Window entity(it.entity(i));
            entity.draw(window);
        });

        world->observer<components::Window>().event(flecs::OnRemove).each([](flecs::entity e, components::Window &window) {
            if (window.renderer_data.renderer) {
                SDL_DestroyRenderer(window.renderer_data.renderer);
                window.renderer_data.renderer = nullptr;
            }

            if (window.window) {
                SDL_DestroyWindow(window.window);
                window.window = nullptr;
            }

            if (window.renderer_data.text_engine) {
                TTF_DestroyRendererTextEngine(window.renderer_data.text_engine);
                window.renderer_data.text_engine = nullptr;
            }

            if (window.renderer_data.fonts) {
                SDL_free(window.renderer_data.fonts);
                window.renderer_data.fonts = nullptr;
            }

            SDL_free(window.clay_arena.memory);

            if (window.close_callback.has_value()) {
                window.close_callback.value()();
            }
        });
    }

    static inline Clay_Dimensions measureText(Clay_StringSlice text, Clay_TextElementConfig *config, void *data)
    {
        TTF_Font **fonts = static_cast<TTF_Font **>(data);
        TTF_Font *font = fonts[config->fontId];
        int width, height;

        TTF_SetFontSize(font, config->fontSize);
        if (!TTF_GetStringSize(font, text.chars, text.length, &width, &height)) {
            spdlog::error("Failed to measure text: {}", SDL_GetError());
        }

        return Clay_Dimensions{ static_cast<float>(width), static_cast<float>(height) };
    }

    void Window::initialize()
    {
        Entity::initialize();

        setComponent<components::Window>({ "Atmo Engine", { 800, 600 } });
        auto window = p_handle.get_ref<components::Window>();

        static constexpr SDL_WindowFlags flags = SDL_WINDOW_HIGH_PIXEL_DENSITY;

        window->window = SDL_CreateWindow(window->title.c_str(), window->size.x, window->size.y, flags);

        if (!window->window) {
            spdlog::error("Failed to create SDL window: {}", SDL_GetError());
            throw std::runtime_error("Failed to create SDL window");
        }

        window->renderer_data.renderer = SDL_CreateRenderer(window->window, "metal,vulkan");

        if (!window->renderer_data.renderer) {
            spdlog::error("Failed to create SDL renderer: {}", SDL_GetError());
            throw std::runtime_error("Failed to create SDL renderer");
        }

        SDL_SetWindowResizable(window->window, true);

        window->renderer_data.text_engine = TTF_CreateRendererTextEngine(window->renderer_data.renderer);
        if (!window->renderer_data.text_engine) {
            spdlog::error("Failed to create text engine from renderer: {}", SDL_GetError());
            throw std::runtime_error("Failed to create text engine from renderer");
        }

        window->renderer_data.fonts = (TTF_Font **)SDL_calloc(1, sizeof(TTF_Font *));
        if (!window->renderer_data.fonts) {
            spdlog::error("Failed to allocate memory for the font array: {}", SDL_GetError());
            throw std::runtime_error("Failed to allocate memory for the font array");
        }

        auto totalMemSize = Clay_MinMemorySize();
        window->clay_arena = Clay_CreateArenaWithCapacityAndMemory(totalMemSize, SDL_malloc(totalMemSize));

        Clay_Initialize(window->clay_arena, { (float)window->size.x, (float)window->size.y }, { .errorHandlerFunction = [](Clay_ErrorData errorData) {
                            spdlog::error("Clay error: {}", errorData.errorText.chars);
                        } });

        Clay_SetMeasureTextFunction(measureText, window->renderer_data.fonts);
    }

    void Window::setName(const std::string &name)
    {
        auto window = p_handle.get_ref<components::Window>();

        if (SDL_SetWindowTitle(window->window, name.c_str())) {
            window->title = name;
        } else {
            spdlog::error("Failed to set window title: {}", SDL_GetError());
        }
    }

    void Window::setSize(const core::types::Vector2i &size)
    {
        auto window = p_handle.get_ref<components::Window>();

        if (SDL_SetWindowSize(window->window, size.x, size.y)) {
            window->size = size;
        } else {
            spdlog::error("Failed to set window size: {}", SDL_GetError());
        }
    }

    void Window::focus()
    {
        auto window = p_handle.get_ref<components::Window>();

        if (!SDL_RaiseWindow(window->window)) {
            spdlog::error("Failed to focus window: {}", SDL_GetError());
        }
    }

    void Window::pollEvents(float deltaTime)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_EVENT_QUIT:
                    p_handle.destruct();
                    break;
                case SDL_EVENT_WINDOW_RESIZED:
                    Clay_SetLayoutDimensions({ (float)event.window.data1, (float)event.window.data2 });
                    break;
                case SDL_EVENT_MOUSE_MOTION:
                    Clay_SetPointerState({ event.motion.x, event.motion.y }, event.motion.state & SDL_BUTTON_LMASK);
                    break;
                default:
                    auto default_event = atmo::core::event::EventRegistry::Create<atmo::core::event::events::InputEvent>("Event::SDLEvent::InputEvent");
                    default_event->sdl_event = event;
                    atmo::core::event::EventRegistry::Dispatch(default_event);
                    break;
            };
        }
    }

    void Window::beginDraw(components::Window &window)
    {
        SDL_RenderClear(window.renderer_data.renderer);
        SDL_SetRenderDrawBlendMode(window.renderer_data.renderer, SDL_BLENDMODE_BLEND);
    }

    void Window::draw(components::Window &window)
    {
        if (core::InputManager::IsJustPressed("ui_click")) {
            auto pos = core::InputManager::GetMousePosition();
            Clay_SetPointerState({ pos.x, pos.y }, true);
        }

        if (core::InputManager::IsJustReleased("ui_click")) {
            auto pos = core::InputManager::GetMousePosition();
            Clay_SetPointerState({ pos.x, pos.y }, false);
        }

        auto scroll = core::InputManager::GetScrollDelta("ui_scroll");
        if (scroll.first.x != 0 || scroll.first.y != 0)
            Clay_UpdateScrollContainers(true, { scroll.first.x, scroll.first.y }, scroll.second);

        Clay_BeginLayout();

        p_handle.children([this](flecs::entity child) { declareEntityUi(child); });

        auto commands = Clay_EndLayout();

        SDL_SetRenderDrawColor(window.renderer_data.renderer, 0, 0, 0, 255);

        SDL_Clay_RenderClayCommands(&window.renderer_data, &commands);

        SDL_RenderPresent(window.renderer_data.renderer);
    }

    Clay_ElementId Window::getIdForEntity(flecs::entity e)
    {
        std::string path = std::format("#{}", e.id());
        Clay_String s{ false, static_cast<std::int32_t>(path.size()), path.c_str() };
        return Clay_GetElementId(s);
    }

    Clay_ElementDeclaration Window::buildDecl(flecs::entity e)
    {
        Clay_ElementDeclaration d{};

        d.id = getIdForEntity(e);

        return d;
    }

    void Window::declareEntityUi(flecs::entity e)
    {
        Clay_ElementDeclaration decl = buildDecl(e);

        CLAY(decl)
        {
            e.children([this](flecs::entity child) { declareEntityUi(child); });
        }
    }

    SDL_Texture *Window::getTextureFromHandle(const std::string &path)
    {
        auto window = p_handle.get_ref<components::Window>();

        if (window->texture_cache.find(path) != window->texture_cache.end()) {
            return window->texture_cache[path];
        }

        atmo::core::resource::ResourceRef<SDL_Surface> res = atmo::core::resource::ResourceManager::GetInstance().getResource<SDL_Surface>(path);
        std::shared_ptr<SDL_Surface> surface = res.get();

        SDL_Texture *texture = SDL_CreateTextureFromSurface(window->renderer_data.renderer, surface.get());
        window->texture_cache[path] = texture;
        return texture;
    }

    void Window::onClose(std::function<void()> callback)
    {
        auto window = p_handle.get_ref<components::Window>();
        window->close_callback = callback;
    }
} // namespace atmo::core::ecs::entities

REGISTER_ENTITY(entities::Window);
ATMO_REGISTER_COMPONENT(atmo::core::components::Window)
