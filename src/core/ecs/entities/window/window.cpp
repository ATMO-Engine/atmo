#include "window.hpp"

#include "SDL3/SDL_stdinc.h"
#include "core/args/arg_manager.hpp"
#include "core/ecs/components.hpp"
#include "core/ecs/entities/ui/ui.hpp"
#include "core/ecs/entity_registry.hpp"
#include "core/ecs/world_context.hpp"
#include "core/input/input_manager.hpp"
#include "core/types.hpp"
#include "impl/clay_canvas_renderer.hpp"
#include "meta/auto_register.hpp"
#include "project/project_manager.hpp"
#include "server/rendering/rendering_server.hpp"
#include "spdlog/spdlog.h"

#define CLAY_IMPLEMENTATION
#include <clay.h>

namespace atmo::core::ecs::entities
{
    void Window::RegisterSystems(flecs::world *world)
    {
        const components::WorldContext *ctx = world->try_get<components::WorldContext>();
        if (ctx && ctx->is_editor_isolated)
            return;

        world->system("PollEvents").kind(flecs::PreUpdate).run([](flecs::iter &it) { server::rendering::RenderingServer::Instance().pollEvents(); });

        world->system<components::Window>("Draw").kind(flecs::PostUpdate).each([](flecs::iter &it, size_t i, components::Window &window) {
            entities::Window entity(it.entity(i));
            entity.draw(window);
        });

        world->observer<components::Window>().event(flecs::OnRemove).each([](flecs::entity e, components::Window &window) {
            auto &rendering_server = server::rendering::RenderingServer::Instance();
            if (auto *ctx = e.world().try_get_mut<components::WorldContext>();
                ctx && ctx->viewport.isValid() && ctx->viewport == rendering_server.windowGetViewport(window.rid)) {
                rendering_server.canvasFree(ctx->canvas);
                ctx->canvas = {};
                ctx->viewport = {};
            }

            rendering_server.canvasItemFree(window.ui_item);
            rendering_server.canvasFree(window.ui_canvas);
            rendering_server.windowFree(window.rid);
            window.ui_item = {};
            window.ui_canvas = {};
            window.rid = {};

            if (window.clay_context && Clay_GetCurrentContext() == window.clay_context)
                Clay_SetCurrentContext(nullptr);
            window.clay_context = nullptr;
            SDL_free(window.clay_arena.memory);
            window.clay_arena = {};

            if (window.close_callback.has_value()) {
                window.close_callback.value()();
            }
        });
    }

    static void RefreshWorldViewport(flecs::entity handle)
    {
        auto &rendering_server = server::rendering::RenderingServer::Instance();
        const auto *window = handle.try_get<components::Window>();
        auto *ctx = handle.world().try_get_mut<components::WorldContext>();
        if (!window || !ctx || !ctx->viewport.isValid() || ctx->viewport != rendering_server.windowGetViewport(window->rid))
            return;

        const types::Vector2i size = rendering_server.windowGetSize(window->rid);
        ctx->viewport_size = { static_cast<float>(size.x), static_cast<float>(size.y) };
        ctx->viewport_dpi = rendering_server.windowGetDpiScale(window->rid);
    }

    void Window::initialize()
    {
        Entity::initialize();

        setComponent<components::Window>({ "Atmo Engine", project::ProjectManager::GetSettings().window.size });
        auto &window = getComponentMutable<components::Window>();

        if (args::ArgManager::Get<bool>("--headless")) {
            window.headless = true;
            return;
        }

        auto &rendering_server = server::rendering::RenderingServer::Instance();
        window.rid = rendering_server.windowCreate({ .title = window.title, .size = window.size });
        if (!window.rid.isValid()) {
            spdlog::error("Failed to create window");
            return;
        }

        window.dpi_scale = rendering_server.windowGetDpiScale(window.rid);

        const server::rendering::ViewportRid viewport = rendering_server.windowGetViewport(window.rid);
        rendering_server.viewportSetClearColor(viewport, project::ProjectManager::GetSettings().window.background_color);
        rendering_server.viewportSetBorderColor(viewport, project::ProjectManager::GetSettings().window.letterbox_color);
        if (auto *ctx = p_handle.world().try_get_mut<components::WorldContext>(); ctx && !ctx->is_editor_isolated && !ctx->canvas.isValid()) {
            ctx->canvas = rendering_server.canvasCreate();
            ctx->viewport = viewport;
            ctx->viewport_size = { static_cast<float>(window.size.x), static_cast<float>(window.size.y) };
            ctx->viewport_dpi = window.dpi_scale;
            rendering_server.viewportSetCanvas(viewport, ctx->canvas);
        }
        window.ui_canvas = rendering_server.canvasCreate();
        window.ui_item = rendering_server.canvasItemCreate(window.ui_canvas);
        rendering_server.viewportAttachCanvas(viewport, window.ui_canvas, 1);
        rendering_server.windowSetEventCallback(window.rid, [handle = p_handle](const server::rendering::WindowEvent &event) {
            switch (event.type) {
                case server::rendering::WindowEventType::QuitRequested:
                    handle.destruct();
                    break;
                case server::rendering::WindowEventType::Resized:
                    {
                        entities::Window entity(handle);
                        if (Clay_Context *context = entity.getComponent<components::Window>().clay_context) {
                            Clay_SetCurrentContext(context);
                            Clay_SetLayoutDimensions({ static_cast<float>(event.size.x), static_cast<float>(event.size.y) });
                        }
                        RefreshWorldViewport(handle);
                        break;
                    }
                case server::rendering::WindowEventType::DisplayScaleChanged:
                    {
                        entities::Window entity(handle);
                        auto &component = entity.getComponentMutable<components::Window>();
                        component.dpi_scale = server::rendering::RenderingServer::Instance().windowGetDpiScale(component.rid);
                        RefreshWorldViewport(handle);
                        break;
                    }
            }
        });

        auto totalMemSize = Clay_MinMemorySize();
        window.clay_arena = Clay_CreateArenaWithCapacityAndMemory(totalMemSize, SDL_malloc(totalMemSize));

        window.clay_context =
            Clay_Initialize(window.clay_arena, { (float)window.size.x, (float)window.size.y }, { .errorHandlerFunction = [](Clay_ErrorData errorData) {
                                spdlog::error("Clay error: {}", errorData.errorText.chars);
                            } });

        Clay_SetMeasureTextFunction(impl::ClayMeasureText, nullptr);
    }

    bool Window::setTitle(const std::string &name)
    {
        auto window = p_handle.get_ref<components::Window>();

        if (window->headless)
            return false;

        if (!server::rendering::RenderingServer::Instance().windowSetTitle(window->rid, name))
            return false;

        window->title = name;
        return true;
    }

    void Window::setSize(const core::types::Vector2i &size)
    {
        auto window = p_handle.get_ref<components::Window>();

        if (server::rendering::RenderingServer::Instance().windowSetSize(window->rid, size))
            window->size = size;
    }

    types::Vector2 Window::getDPIScale() const noexcept
    {
        return getComponent<components::Window>().dpi_scale;
    }

    void Window::focus()
    {
        auto window = p_handle.get_ref<components::Window>();

        server::rendering::RenderingServer::Instance().windowFocus(window->rid);
    }

    void Window::draw(components::Window &window)
    {
        if (window.headless || !window.clay_context)
            return;

        Clay_SetCurrentContext(window.clay_context);

        bool clickState = false;
        auto pos = core::InputManager::GetMousePosition();

        if (core::InputManager::IsJustPressed("ui_click") || core::InputManager::IsPressed("ui_click")) {
            clickState = true;
        }
        Clay_SetPointerState({ pos.x, pos.y }, clickState);

        auto scroll = core::InputManager::GetScrollDelta("ui_scroll");
        if (scroll.first.x != 0 || scroll.first.y != 0)
            Clay_UpdateScrollContainers(true, { scroll.first.x, scroll.first.y }, scroll.second);

        Clay_BeginLayout();
        for (auto &child : getChildren(true)) {
            auto wrapped = EntityRegistry::Wrap(child);
            auto *ui = dynamic_cast<entities::UI *>(wrapped.get());
            if (ui && !child.getParent().hasComponent<components::UI>())
                ui->internalDraw();
        }
        const Clay_RenderCommandArray commands = Clay_EndLayout();

        server::rendering::CanvasCommandWriter writer;
        impl::ClayRenderCommands(commands, writer, window.dpi_scale.x);
        server::rendering::RenderingServer::Instance().canvasItemSetCommands(window.ui_item, std::move(writer));
    }

    void Window::onClose(std::function<void()> callback)
    {
        auto window = p_handle.get_ref<components::Window>();
        window->close_callback = callback;
    }

    // TODO: remove once Clay is replaced with an in-house UI system.
    void Window::primeScrollContainers()
    {
        if (Clay_Context *context = getComponent<components::Window>().clay_context)
            Clay_SetCurrentContext(context);
        Clay_UpdateScrollContainers(true, { 0.0f, 0.0001f }, 0.0f);
    }
} // namespace atmo::core::ecs::entities

ATMO_REGISTER_ENTITY(entities::Window);
ATMO_REGISTER_COMPONENT(atmo::core::components::Window);
