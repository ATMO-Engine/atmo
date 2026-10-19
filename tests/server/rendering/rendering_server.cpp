#include <algorithm>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "SDL3/SDL.h"
#include "core/event/event_registry.hpp"
#include "core/event/events/sdl_event/input_event/input_event.hpp"
#include "offscreen_video.hpp"
#include "server/rendering/api/null/null_platform.hpp"
#include "server/rendering/rendering_server.hpp"

using namespace atmo::server::rendering;

namespace
{
    struct Server {
        explicit Server(RenderingSettings settings = { .render_thread = false, .legacy_sdl_renderer = false })
        {
            REQUIRE(RenderingServer::Instance().init(settings));
        }

        ~Server()
        {
            RenderingServer::Instance().shutdown();
        }

        RenderingServer &operator*() const
        {
            return RenderingServer::Instance();
        }

        RenderingServer *operator->() const
        {
            return &RenderingServer::Instance();
        }

        [[nodiscard]] NullPlatform &null() const
        {
            return dynamic_cast<NullPlatform &>(RenderingServer::Instance().platform());
        }
    };
} // namespace

TEST_CASE("the server starts on the Null platform and shuts down cleanly", "[rendering][server]")
{
    {
        Server server;
        REQUIRE(server->isInitialized());
        REQUIRE(server->platformKind() == PlatformKind::Null);
        REQUIRE(server->init());
    }

    REQUIRE_FALSE(RenderingServer::Instance().isInitialized());
}

TEST_CASE("sync runs one platform frame", "[rendering][server]")
{
    Server server;

    server->sync();
    server->sync();
    server->sync();

    REQUIRE(server.null().frameCount() == 3);
}

TEST_CASE("calls before init are harmless", "[rendering][server]")
{
    auto &server = RenderingServer::Instance();
    REQUIRE_FALSE(server.isInitialized());

    REQUIRE_FALSE(server.windowCreate({ .title = "unused", .size = { 64, 64 } }).isValid());
    server.windowFree(WindowRid{});
    server.pollEvents();
    server.sync();
    REQUIRE_FALSE(server.windowSetTitle(WindowRid{}, "unused"));
}

TEST_CASE("window swapchains follow the command queue", "[rendering][server][window]")
{
    OffscreenVideo video;
    if (!video.available)
        SKIP("SDL offscreen video driver unavailable: " << SDL_GetError());

    Server server;

    WindowRid window = server->windowCreate({ .title = "test", .size = { 320, 200 } });
    REQUIRE(window.isValid());
    REQUIRE(server->windowIsAlive(window));
    REQUIRE(server->legacySdlWindow(window) != nullptr);
    REQUIRE(server->windowGetSize(window).x == 320);
    REQUIRE(server->windowGetSize(window).y == 200);

    REQUIRE(server->pendingCommandCount() == 1);
    REQUIRE(server.null().swapchainCount() == 0);

    server->sync();
    REQUIRE(server.null().swapchainCount() == 1);

    server->windowFree(window);
    REQUIRE_FALSE(server->windowIsAlive(window));
    REQUIRE(server.null().swapchainCount() == 1);

    server->sync();
    REQUIRE(server.null().swapchainCount() == 0);
}

TEST_CASE("inline mode creates swapchains immediately", "[rendering][server][window]")
{
    OffscreenVideo video;
    if (!video.available)
        SKIP("SDL offscreen video driver unavailable: " << SDL_GetError());

    Server server({ .inline_commands = true, .render_thread = false, .legacy_sdl_renderer = false });

    WindowRid window = server->windowCreate({ .title = "test", .size = { 128, 128 } });
    REQUIRE(window.isValid());
    REQUIRE(server->pendingCommandCount() == 0);
    REQUIRE(server.null().swapchainCount() == 1);
}

TEST_CASE("stale window RIDs are rejected", "[rendering][server][window]")
{
    OffscreenVideo video;
    if (!video.available)
        SKIP("SDL offscreen video driver unavailable: " << SDL_GetError());

    Server server;

    WindowRid first = server->windowCreate({ .title = "first", .size = { 64, 64 } });
    server->windowFree(first);
    server->sync();

    WindowRid second = server->windowCreate({ .title = "second", .size = { 64, 64 } });
    REQUIRE(second.slot() == first.slot());
    REQUIRE(second != first);

    REQUIRE_FALSE(server->windowSetTitle(first, "stale"));
    REQUIRE_FALSE(server->windowSetSize(first, { 32, 32 }));
    REQUIRE(server->legacySdlWindow(first) == nullptr);
    server->windowFree(first);

    REQUIRE(server->windowIsAlive(second));
    REQUIRE(server->windowSetTitle(second, "renamed"));
}

TEST_CASE("resizing a window resizes its swapchain and notifies the owner", "[rendering][server][window]")
{
    OffscreenVideo video;
    if (!video.available)
        SKIP("SDL offscreen video driver unavailable: " << SDL_GetError());

    Server server;

    WindowRid window = server->windowCreate({ .title = "resize", .size = { 320, 200 } });
    server->sync();

    std::vector<WindowEvent> received;
    server->windowSetEventCallback(window, [&received](const WindowEvent &event) { received.push_back(event); });

    REQUIRE(server->windowSetSize(window, { 640, 360 }));
    server->pollEvents();
    server->sync();

    REQUIRE_FALSE(received.empty());
    REQUIRE(received.back().type == WindowEventType::Resized);
    REQUIRE(received.back().size.x == 640);
    REQUIRE(received.back().size.y == 360);

    auto sizes = server.null().swapchainSizes();
    REQUIRE(sizes.size() == 1);
    REQUIRE(sizes.front().x == 640);
    REQUIRE(sizes.front().y == 360);
}

TEST_CASE("a quit request reaches every window and may free it from the callback", "[rendering][server][window]")
{
    OffscreenVideo video;
    if (!video.available)
        SKIP("SDL offscreen video driver unavailable: " << SDL_GetError());

    Server server;

    WindowRid a = server->windowCreate({ .title = "a", .size = { 64, 64 } });
    WindowRid b = server->windowCreate({ .title = "b", .size = { 64, 64 } });

    int quit_count = 0;
    auto close_on_quit = [&quit_count](WindowRid window) {
        return [&quit_count, window](const WindowEvent &event) {
            if (event.type != WindowEventType::QuitRequested)
                return;
            ++quit_count;
            RenderingServer::Instance().windowFree(window);
        };
    };
    server->windowSetEventCallback(a, close_on_quit(a));
    server->windowSetEventCallback(b, close_on_quit(b));

    SDL_Event quit{};
    quit.type = SDL_EVENT_QUIT;
    REQUIRE(SDL_PushEvent(&quit));
    server->pollEvents();

    REQUIRE(quit_count == 2);
    REQUIRE_FALSE(server->windowIsAlive(a));
    REQUIRE_FALSE(server->windowIsAlive(b));

    server->sync();
    REQUIRE(server.null().swapchainCount() == 0);
}

TEST_CASE("events that are not window events are forwarded as input events", "[rendering][server][window]")
{
    OffscreenVideo video;
    if (!video.available)
        SKIP("SDL offscreen video driver unavailable: " << SDL_GetError());

    Server server;

    using atmo::core::event::EventRegistry;
    using atmo::core::event::events::InputEvent;

    std::vector<SDL_EventType> forwarded;
    auto id =
        EventRegistry::SetCallBack<InputEvent>([&forwarded](InputEvent *event) { forwarded.push_back(static_cast<SDL_EventType>(event->sdl_event.type)); });

    SDL_Event key{};
    key.type = SDL_EVENT_KEY_DOWN;
    REQUIRE(SDL_PushEvent(&key));
    server->pollEvents();

    EventRegistry::RemoveCallBack<InputEvent>(id);

    REQUIRE(std::find(forwarded.begin(), forwarded.end(), SDL_EVENT_KEY_DOWN) != forwarded.end());
}

TEST_CASE("each presentable window is cleared and presented once per frame", "[rendering][server][window]")
{
    OffscreenVideo video;
    if (!video.available)
        SKIP("SDL offscreen video driver unavailable: " << SDL_GetError());

    Server server;

    server->windowCreate({ .title = "a", .size = { 64, 64 } });
    server->windowCreate({ .title = "b", .size = { 32, 32 } });
    server->sync();
    REQUIRE(server.null().presentCount() == 2);

    server->sync();
    REQUIRE(server.null().presentCount() == 4);
}

TEST_CASE("the legacy renderer is only offered when the platform does not present", "[rendering][server]")
{
    {
        Server server({ .render_thread = false, .legacy_sdl_renderer = true });
        REQUIRE(server->legacyRendererEnabled());
    }
    {
        Server server({ .render_thread = false, .legacy_sdl_renderer = false });
        REQUIRE_FALSE(server->legacyRendererEnabled());
    }
}
