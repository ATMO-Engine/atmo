#include <SDL3/SDL.h>
#include <spdlog/spdlog.h>
#include "core/input_manager.hpp"

#include "window.hpp"

#define CLAY_IMPLEMENTATION
#include <clay.h>

void SDL_Clay_RenderClayCommands(Clay_SDL3RendererData *rendererData, Clay_RenderCommandArray *rcommands);

atmo::impl::WindowManager::WindowManager(flecs::entity entity)
{
    this->entity = entity;
    const atmo::core::components::Window &window = entity.get<atmo::core::components::Window>();

    SDL_WindowFlags flags = (SDL_WindowFlags)(SDL_WINDOW_HIGH_PIXEL_DENSITY);

    if (!SDL_CreateWindowAndRenderer(window.title.c_str(), window.size.x, window.size.y, flags, &this->window, &rendererData.renderer)) {
        spdlog::error("Failed to create window: {}", SDL_GetError());
        throw std::runtime_error("Failed to create window");
    }

    SDL_SetWindowResizable(this->window, true);

    rendererData.textEngine = TTF_CreateRendererTextEngine(rendererData.renderer);
    if (!rendererData.textEngine) {
        spdlog::error("Failed to create text engine from renderer: {}", SDL_GetError());
        throw std::runtime_error("Failed to create text engine from renderer");
    }

    rendererData.fonts = (TTF_Font **)SDL_calloc(1, sizeof(TTF_Font *));
    if (!rendererData.fonts) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to allocate memory for the font array: %s", SDL_GetError());
        spdlog::error("Failed to allocate memory for the font array: {}", SDL_GetError());
        throw std::runtime_error("Failed to allocate memory for the font array");
    }

    auto totalMemSize = Clay_MinMemorySize();
    clay_arena = Clay_CreateArenaWithCapacityAndMemory(totalMemSize, SDL_malloc(totalMemSize));

    Clay_Initialize(clay_arena, { (float)window.size.x, (float)window.size.y }, { .errorHandlerFunction = [](Clay_ErrorData errorData) {
                        spdlog::error("Clay error: {}", errorData.errorText.chars);
                    } });

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

    if (main_window == entity)
        entity.world().quit();
}

void atmo::impl::WindowManager::pollEvents(float deltaTime)
{
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_EVENT_QUIT:
                entity.destruct();
                break;
            case SDL_EVENT_WINDOW_RESIZED:
                Clay_SetLayoutDimensions({ (float)event.window.data1, (float)event.window.data2 });
                break;
            case SDL_EVENT_MOUSE_MOTION:
                Clay_SetPointerState({ event.motion.x, event.motion.y }, event.motion.state & SDL_BUTTON_LMASK);
                break;
            default:
                core::InputManager::ProcessEvent(event, deltaTime);
                break;
        };
    }
}

void atmo::impl::WindowManager::draw()
{
    if (core::InputManager::IsJustPressed("ui_click")) {
        auto pos = core::InputManager::GetMousePosition();
        Clay_SetPointerState({ pos.x, pos.y }, true);
        spdlog::info("Click at {}, {}", pos.x, pos.y);
    }

    if (core::InputManager::IsJustReleased("ui_click")) {
        auto pos = core::InputManager::GetMousePosition();
        Clay_SetPointerState({ pos.x, pos.y }, false);
        spdlog::info("Release at {}, {}", pos.x, pos.y);
    }

    auto scroll = core::InputManager::GetScrollDelta("ui_scroll");
    if (scroll.first.x != 0 || scroll.first.y != 0)
        Clay_UpdateScrollContainers(true, { scroll.first.x, scroll.first.y }, scroll.second);

    Clay_BeginLayout();

    entity.children([this](flecs::entity child) { DeclareEntityUI(child); });

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

void atmo::impl::WindowManager::focus() noexcept
{
    if (window) {
        SDL_RaiseWindow(window);
    }
}

void atmo::impl::WindowManager::make_main() noexcept
{
    if (window && main_window != entity) {
        main_window = entity;
    }
}

atmo::core::types::vector2i atmo::impl::WindowManager::getSize() const noexcept
{
    if (window) {
        int w, h;
        SDL_GetWindowSize(window, &w, &h);
        return { w, h };
    }
    return { -1, -1 };
}

std::string atmo::impl::WindowManager::getTitle() const noexcept
{
    if (window)
        return std::string(SDL_GetWindowTitle(window));
    return "";
}

Clay_ElementDeclaration atmo::impl::WindowManager::BuildDecl(flecs::entity e)
{
    Clay_ElementDeclaration d{};

    d.id = getIdForEntity(e);


    // if (e.has<core::components::UIStack>()) {
    //     auto stack = e.get<core::components::UIStack>();
    //     d.layout.layoutDirection = (stack.direction == core::components::UILayoutDirection::Horizontal)
    //         ? CLAY_LEFT_TO_RIGHT
    //         : CLAY_TOP_TO_BOTTOM;
    //     d.layout.childGap = stack.childGap;
    //     d.layout.padding = { static_cast<uint16_t>(stack.padding.x),
    //                          static_cast<uint16_t>(stack.padding.y),
    //                          static_cast<uint16_t>(stack.padding.z),
    //                          static_cast<uint16_t>(stack.padding.w) };
    // }

    // if (e.has<core::components::UITransform>()) {
    //     auto t = e.get<core::components::UITransform>();
    //     if (t.visible) {
    //         d.layout.sizing.width = CLAY_SIZING_FIXED(t.width);
    //         d.layout.sizing.height = CLAY_SIZING_FIXED(t.height);
    //         if (t.floating) {
    //             d.floating.attachTo = CLAY_ATTACH_TO_ROOT;
    //             d.floating.offset = { t.x, t.y };
    //         }
    //     }
    // }

    // if (e.has<core::components::UIRect>()) {
    //     auto rect = e.get<core::components::UIRect>();
    //     d.backgroundColor = { rect.color.r * 255, rect.color.g * 255, rect.color.b * 255, rect.color.a * 255 };
    //     d.border = { .color = { rect.border_color.r * 255,
    //                             rect.border_color.g * 255,
    //                             rect.border_color.b * 255,
    //                             rect.border_color.a * 255 },
    //                  .width = { static_cast<uint16_t>(rect.border_thickness.x),
    //                             static_cast<uint16_t>(rect.border_thickness.y),
    //                             static_cast<uint16_t>(rect.border_thickness.z),
    //                             static_cast<uint16_t>(rect.border_thickness.w) } };
    //     d.cornerRadius = { rect.corner_radius.x, rect.corner_radius.y, rect.corner_radius.z, rect.corner_radius.w };
    // }

    // if (e.has<core::components::UIScroll>()) {
    //     auto scr = e.get<core::components::UIScroll>();
    //     d.clip.horizontal = scr.horizontal;
    //     d.clip.vertical = scr.vertical;
    //     d.clip.childOffset = { scr.scroll_x, scr.scroll_y };
    // }

    // if (e.has<core::components::UIImage>()) {
    //     auto img = e.get<core::components::UIImage>();
    //     // TODO use ressource management system here.
    //     d.image.imageData = nullptr;
    //     // Optionally: ic.cornerRadius / backgroundColor for tint/backing rect
    // }

    // // TODO: floating, z-index, aspect-ratio, etc…

    return d;
}

Clay_ElementId atmo::impl::WindowManager::getIdForEntity(flecs::entity e)
{
    std::string path = std::format("#{}", e.id());
    Clay_String s{ false, (int32_t)path.size(), path.c_str() };
    return Clay_GetElementId(s);
}

void atmo::impl::WindowManager::DeclareEntityUI(flecs::entity e)
{
    Clay_ElementDeclaration decl = BuildDecl(e);

    CLAY(decl)
    {
        // if (e.has<core::components::UIText>()) {
        //     auto txt = e.get<core::components::UIText>();
        //     Clay_TextElementConfig cfg{};
        //     cfg.fontSize = (uint16_t)txt.font_size;
        //     cfg.textColor = { txt.color.r * 255, txt.color.g * 255, txt.color.b * 255, txt.color.a * 255 };
        //     Clay_String str{ false, (int32_t)txt.content.size(), txt.content.c_str() };
        //     CLAY_TEXT(str, &cfg);
        // }

        e.children([this](flecs::entity child) { DeclareEntityUI(child); });
    }
}
