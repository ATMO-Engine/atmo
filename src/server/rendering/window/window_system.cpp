#include "window_system.hpp"

#include <string>

#include "SDL3/SDL_error.h"
#include "SDL3/SDL_properties.h"
#include "spdlog/spdlog.h"

namespace atmo::server::rendering
{
    WindowSystem::~WindowSystem()
    {
        destroyAll();
    }

    bool WindowSystem::create(WindowRid rid, const WindowDesc &desc, SDL_WindowFlags flags, bool metal_view)
    {
        flags |= SDL_WINDOW_HIGH_PIXEL_DENSITY;
        if (desc.resizable)
            flags |= SDL_WINDOW_RESIZABLE;

        SDL_PropertiesID props = SDL_CreateProperties();
        SDL_SetStringProperty(props, SDL_PROP_WINDOW_CREATE_TITLE_STRING, desc.title.c_str());
        SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, desc.size.x);
        SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, desc.size.y);
        SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_FLAGS_NUMBER, static_cast<Sint64>(flags));
        if (!(flags & (SDL_WINDOW_OPENGL | SDL_WINDOW_METAL | SDL_WINDOW_VULKAN)))
            SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_EXTERNAL_GRAPHICS_CONTEXT_BOOLEAN, true);

        SDL_Window *window = SDL_CreateWindowWithProperties(props);
        SDL_DestroyProperties(props);
        if (!window) {
            spdlog::error("Failed to create window: {}", SDL_GetError());
            return false;
        }

        State state{ .rid = rid, .window = window };
#if defined(__APPLE__)
        if (metal_view) {
            state.metal_view = SDL_Metal_CreateView(window);
            if (!state.metal_view) {
                spdlog::error("Failed to create a Metal view: {}", SDL_GetError());
                SDL_DestroyWindow(window);
                return false;
            }
        }
#endif
        RefreshMetrics(state);

        m_by_sdl_id[SDL_GetWindowID(window)] = rid;
        m_windows[rid.raw()] = std::move(state);
        return true;
    }

    void WindowSystem::destroy(WindowRid rid, std::uint64_t release_after)
    {
        auto it = m_windows.find(rid.raw());
        if (it == m_windows.end())
            return;

        SDL_Window *window = it->second.window;
        m_by_sdl_id.erase(SDL_GetWindowID(window));
        m_pending_destroy.push_back({ .window = window, .metal_view = it->second.metal_view, .release_after = release_after });
        m_windows.erase(it);

        SDL_HideWindow(window);
    }

    void WindowSystem::Release(const PendingDestroy &pending)
    {
        if (pending.metal_view)
            SDL_Metal_DestroyView(pending.metal_view);
        SDL_DestroyWindow(pending.window);
    }

    void WindowSystem::destroyPending(std::uint64_t completed_frame)
    {
        std::erase_if(m_pending_destroy, [completed_frame](const PendingDestroy &pending) {
            if (pending.release_after > completed_frame)
                return false;
            Release(pending);
            return true;
        });
    }

    void WindowSystem::destroyAll()
    {
        for (auto &[key, state] : m_windows) m_pending_destroy.push_back({ .window = state.window, .metal_view = state.metal_view });
        m_windows.clear();
        m_by_sdl_id.clear();
        for (const PendingDestroy &pending : m_pending_destroy) Release(pending);
        m_pending_destroy.clear();
    }

    std::size_t WindowSystem::pendingDestroyCount() const noexcept
    {
        return m_pending_destroy.size();
    }

    bool WindowSystem::contains(WindowRid rid) const
    {
        return find(rid) != nullptr;
    }

    std::size_t WindowSystem::count() const noexcept
    {
        return m_windows.size();
    }

    bool WindowSystem::setTitle(WindowRid rid, std::string_view title)
    {
        State *state = find(rid);
        if (!state)
            return false;

        if (!SDL_SetWindowTitle(state->window, std::string(title).c_str())) {
            spdlog::error("Failed to set window title: {}", SDL_GetError());
            return false;
        }
        return true;
    }

    bool WindowSystem::setSize(WindowRid rid, core::types::Vector2i size)
    {
        State *state = find(rid);
        if (!state)
            return false;

        if (!SDL_SetWindowSize(state->window, size.x, size.y)) {
            spdlog::error("Failed to set window size: {}", SDL_GetError());
            return false;
        }
        return true;
    }

    bool WindowSystem::focus(WindowRid rid)
    {
        State *state = find(rid);
        if (!state)
            return false;

        if (!SDL_RaiseWindow(state->window)) {
            spdlog::error("Failed to focus window: {}", SDL_GetError());
            return false;
        }
        return true;
    }

    void WindowSystem::setEventCallback(WindowRid rid, EventCallback callback)
    {
        if (State *state = find(rid))
            state->callback = std::move(callback);
    }

    core::types::Vector2i WindowSystem::size(WindowRid rid) const
    {
        const State *state = find(rid);
        return state ? state->size : core::types::Vector2i{};
    }

    core::types::Vector2i WindowSystem::pixelSize(WindowRid rid) const
    {
        const State *state = find(rid);
        return state ? state->pixel_size : core::types::Vector2i{};
    }

    core::types::Vector2 WindowSystem::dpiScale(WindowRid rid) const
    {
        const State *state = find(rid);
        return state ? state->dpi_scale : core::types::Vector2{ 1.0f, 1.0f };
    }

    SDL_Window *WindowSystem::sdlWindow(WindowRid rid) const
    {
        const State *state = find(rid);
        return state ? state->window : nullptr;
    }

    NativeWindow WindowSystem::nativeWindow(WindowRid rid) const
    {
        const State *state = find(rid);
        if (!state)
            return {};
        return { .sdl_window = state->window, .metal_layer = state->metal_view ? SDL_Metal_GetLayer(state->metal_view) : nullptr };
    }

    void WindowSystem::pollEvents(const EventObserver &observer, const PassthroughCallback &passthrough)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_EVENT_QUIT:
                    {
                        std::vector<WindowRid> rids;
                        rids.reserve(m_windows.size());
                        for (const auto &[key, state] : m_windows) rids.push_back(state.rid);
                        for (WindowRid rid : rids) dispatch(rid, { .type = WindowEventType::QuitRequested }, observer);
                        break;
                    }
                case SDL_EVENT_WINDOW_RESIZED:
                    {
                        WindowRid rid = findBySdlId(event.window.windowID);
                        if (State *state = find(rid)) {
                            RefreshMetrics(*state);
                            dispatch(rid, { .type = WindowEventType::Resized, .size = { event.window.data1, event.window.data2 } }, observer);
                        }
                        break;
                    }
                case SDL_EVENT_WINDOW_DISPLAY_CHANGED:
                    {
                        WindowRid rid = findBySdlId(event.window.windowID);
                        if (State *state = find(rid)) {
                            RefreshMetrics(*state);
                            dispatch(rid, { .type = WindowEventType::DisplayScaleChanged, .size = state->size }, observer);
                        }
                        break;
                    }
                default:
                    passthrough(event);
                    break;
            }
        }
    }

    WindowSystem::State *WindowSystem::find(WindowRid rid)
    {
        auto it = m_windows.find(rid.raw());
        return it == m_windows.end() ? nullptr : &it->second;
    }

    const WindowSystem::State *WindowSystem::find(WindowRid rid) const
    {
        auto it = m_windows.find(rid.raw());
        return it == m_windows.end() ? nullptr : &it->second;
    }

    WindowRid WindowSystem::findBySdlId(SDL_WindowID id) const
    {
        auto it = m_by_sdl_id.find(id);
        return it == m_by_sdl_id.end() ? WindowRid{} : it->second;
    }

    void WindowSystem::RefreshMetrics(State &state)
    {
        SDL_GetWindowSize(state.window, &state.size.x, &state.size.y);
        SDL_GetWindowSizeInPixels(state.window, &state.pixel_size.x, &state.pixel_size.y);

        if (state.size.x > 0 && state.size.y > 0) {
            state.dpi_scale.x = static_cast<float>(state.pixel_size.x) / static_cast<float>(state.size.x);
            state.dpi_scale.y = static_cast<float>(state.pixel_size.y) / static_cast<float>(state.size.y);
        }
    }

    void WindowSystem::dispatch(WindowRid rid, const WindowEvent &event, const EventObserver &observer)
    {
        State *state = find(rid);
        if (!state)
            return;

        if (observer)
            observer(rid, event);

        state = find(rid);
        if (!state || !state->callback)
            return;

        EventCallback callback = state->callback;
        callback(event);
    }
} // namespace atmo::server::rendering
