#pragma once

#include <cstdint>
#include <functional>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "SDL3/SDL_events.h"
#include "SDL3/SDL_metal.h"
#include "SDL3/SDL_video.h"
#include "core/types.hpp"
#include "server/rendering/platform/rendering_platform.hpp"
#include "server/rendering/rid.hpp"
#include "server/rendering/types.hpp"

namespace atmo::server::rendering
{
    class WindowSystem
    {
    public:
        using EventCallback = std::function<void(const WindowEvent &)>;
        using EventObserver = std::function<void(WindowRid, const WindowEvent &)>;
        using PassthroughCallback = std::function<void(const SDL_Event &)>;

        WindowSystem() = default;
        ~WindowSystem();

        WindowSystem(const WindowSystem &) = delete;
        WindowSystem &operator=(const WindowSystem &) = delete;

        bool create(WindowRid rid, const WindowDesc &desc, SDL_WindowFlags flags, bool metal_view = false);
        void destroy(WindowRid rid, std::uint64_t release_after = 0);
        void destroyPending(std::uint64_t completed_frame);
        void destroyAll();
        [[nodiscard]] std::size_t pendingDestroyCount() const noexcept;

        [[nodiscard]] bool contains(WindowRid rid) const;
        [[nodiscard]] std::size_t count() const noexcept;

        bool setTitle(WindowRid rid, std::string_view title);
        bool setSize(WindowRid rid, core::types::Vector2i size);
        bool focus(WindowRid rid);
        void setEventCallback(WindowRid rid, EventCallback callback);

        [[nodiscard]] core::types::Vector2i size(WindowRid rid) const;
        [[nodiscard]] core::types::Vector2i pixelSize(WindowRid rid) const;
        [[nodiscard]] core::types::Vector2 dpiScale(WindowRid rid) const;
        [[nodiscard]] SDL_Window *sdlWindow(WindowRid rid) const;
        [[nodiscard]] NativeWindow nativeWindow(WindowRid rid) const;

        void pollEvents(const EventObserver &observer, const PassthroughCallback &passthrough);

    private:
        struct State {
            WindowRid rid;
            SDL_Window *window = nullptr;
            SDL_MetalView metal_view = nullptr;
            core::types::Vector2i size{};
            core::types::Vector2i pixel_size{};
            core::types::Vector2 dpi_scale{ 1.0f, 1.0f };
            EventCallback callback;
        };

        State *find(WindowRid rid);
        [[nodiscard]] const State *find(WindowRid rid) const;
        WindowRid findBySdlId(SDL_WindowID id) const;
        static void RefreshMetrics(State &state);
        void dispatch(WindowRid rid, const WindowEvent &event, const EventObserver &observer);

        std::unordered_map<std::uint32_t, State> m_windows;
        std::unordered_map<SDL_WindowID, WindowRid> m_by_sdl_id;
        struct PendingDestroy {
            SDL_Window *window = nullptr;
            SDL_MetalView metal_view = nullptr;
            std::uint64_t release_after = 0;
        };

        static void Release(const PendingDestroy &pending);

        std::vector<PendingDestroy> m_pending_destroy;
    };
} // namespace atmo::server::rendering
