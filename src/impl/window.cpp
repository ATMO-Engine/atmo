#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <spdlog/spdlog.h>
#include <stdbool.h>
#include "clay.h"
#include "core/ecs/components.hpp"
#include "core/event/event_dispatcher.hpp"
#include "core/input/input_manager.hpp"

#include "window.hpp"

#define CLAY_IMPLEMENTATION
#include <clay.h>

void SDL_Clay_RenderClayCommands(Clay_SDLRendererData *rendererData, Clay_RenderCommandArray *rcommands);
static Clay_Dimensions SDL_MeasureText(Clay_StringSlice text, Clay_TextElementConfig *config, void *userData)
{
    Clay_Dimensions result{ 0.0f, 0.0f };

    if (!config || !userData || !text.chars || text.length <= 0) {
        return result;
    }

    TTF_Font **fonts = static_cast<TTF_Font **>(userData);

    if (config->fontId < 0 || config->fontId > 0) {
        spdlog::warn("MeasureText: invalid fontId={} (only fontId=0 is valid for now)", (int)config->fontId);
        return result;
    }

    TTF_Font *font = fonts[0];
    if (!font) {
        spdlog::warn("MeasureText: font[0] is null");
        return result;
    }

    TTF_SetFontSize(font, config->fontSize);

    int w = 0, h = 0;
    if (!TTF_GetStringSize(font, text.chars, text.length, &w, &h)) {
        spdlog::error("Failed to measure text: {}", SDL_GetError());
        return result;
    }

    result.width = (float)w;
    result.height = (float)h;
    return result;
}

atmo::impl::WindowManager::WindowManager(flecs::entity entity)
{
    this->entity = entity;
    const atmo::core::components::Window &window = entity.get<atmo::core::components::Window>();

    SDL_WindowFlags flags = SDL_WINDOW_HIGH_PIXEL_DENSITY;

    m_window = SDL_CreateWindow(window.title.c_str(), window.size.x, window.size.y, flags);

    if (!m_window) {
        spdlog::error("Failed to create SDL window: {}", SDL_GetError());
        throw std::runtime_error("Failed to create SDL window");
    }

    m_rendererData.renderer = SDL_CreateRenderer(m_window, "metal,vulkan");

    if (!m_rendererData.renderer) {
        spdlog::error("Failed to create SDL renderer: {}", SDL_GetError());
        throw std::runtime_error("Failed to create SDL renderer");
    }

    SDL_SetWindowResizable(m_window, true);

    m_rendererData.text_engine = TTF_CreateRendererTextEngine(m_rendererData.renderer);
    if (!m_rendererData.text_engine) {
        spdlog::error("Failed to create text engine from renderer: {}", SDL_GetError());
        throw std::runtime_error("Failed to create text engine from renderer");
    }

    m_rendererData.fonts = (TTF_Font **)SDL_calloc(1, sizeof(TTF_Font *));
    if (!m_rendererData.fonts) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to allocate memory for the font array: %s", SDL_GetError());
        spdlog::error("Failed to allocate memory for the font array: {}", SDL_GetError());
        throw std::runtime_error("Failed to allocate memory for the font array");
    }

    m_rendererData.fonts[0] = TTF_OpenFont("/Users/albericdesaegher/taftaf/ATMO/atmo/assets/font.ttf", 24);

    if (!m_rendererData.fonts[0]) {
        spdlog::error("Failed to load font: {}", SDL_GetError());
        throw std::runtime_error("Failed to load font");
    }

    auto totalMemSize = Clay_MinMemorySize();
    m_clayArena = Clay_CreateArenaWithCapacityAndMemory(totalMemSize, SDL_malloc(totalMemSize));

    Clay_Initialize(m_clayArena, { (float)window.size.x, (float)window.size.y }, { .errorHandlerFunction = [](Clay_ErrorData errorData) {
                        spdlog::error("Clay error: {}", errorData.errorText.chars);
                    } });

    Clay_SetMeasureTextFunction(SDL_MeasureText, m_rendererData.fonts);
}

atmo::impl::WindowManager::~WindowManager()
{
    if (m_rendererData.renderer) {
        SDL_DestroyRenderer(m_rendererData.renderer);
        m_rendererData.renderer = nullptr;
    }

    if (m_window) {
        SDL_DestroyWindow(m_window);
        m_window = nullptr;
    }

    SDL_free(m_clayArena.memory);

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
                atmo::core::event::EventDispatcher::Dispatch(new atmo::core::InputManager::InputEvent(event));
                break;
        };
    }
}

void atmo::impl::WindowManager::beginDraw()
{
    SDL_SetRenderDrawColor(m_rendererData.renderer, 0, 0, 0, 255);
    SDL_RenderClear(m_rendererData.renderer);
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

    entity.children([this](flecs::entity child) { declareEntityUi(child); });

    auto commands = Clay_EndLayout();

    SDL_Clay_RenderClayCommands(&m_rendererData, &commands);

    SDL_RenderPresent(m_rendererData.renderer);
}

void atmo::impl::WindowManager::rename(const std::string &name) noexcept
{
    if (m_window)
        SDL_SetWindowTitle(m_window, name.c_str());
}

void atmo::impl::WindowManager::resize(const atmo::core::types::vector2i &size) noexcept
{
    if (m_window)
        SDL_SetWindowSize(m_window, size.x, size.y);
}

void atmo::impl::WindowManager::focus() noexcept
{
    if (m_window) {
        SDL_RaiseWindow(m_window);
    }
}

void atmo::impl::WindowManager::makeMain() noexcept
{
    if (m_window && main_window != entity) {
        main_window = entity;
    }
}

atmo::core::types::vector2i atmo::impl::WindowManager::getSize() const noexcept
{
    if (m_window) {
        int w, h;
        SDL_GetWindowSize(m_window, &w, &h);
        return { w, h };
    }
    return { -1, -1 };
}

std::string atmo::impl::WindowManager::getTitle() const noexcept
{
    if (m_window)
        return std::string(SDL_GetWindowTitle(m_window));
    return "";
}

Clay_ElementDeclaration atmo::impl::WindowManager::buildDecl(flecs::entity e)
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

void atmo::impl::WindowManager::declareEntityUi(flecs::entity e)
{
    Clay_ElementDeclaration decl = buildDecl(e);

    if (e.has<core::components::UI::UI>()) {
        auto &ui = e.get<core::components::UI::UI>();

        if (!ui.visible) {
            return;
        }
        decl.backgroundColor = { ui.modulate.r * 255, ui.modulate.g * 255, ui.modulate.b * 255, ui.modulate.a * 255 };
    }

    if (e.has<core::components::UI::Position>()) {
        auto &pos = e.get<core::components::UI::Position>();

        decl.layout.sizing.width = CLAY_SIZING_FIXED(pos.size.x);
        decl.layout.sizing.height = CLAY_SIZING_FIXED(pos.size.y);
        decl.layout.padding = { pos.padding.left, pos.padding.right, pos.padding.top, pos.padding.bottom };
    }

    if (e.has<core::components::UI::Text>()) {
        decl.layout.sizing.width = CLAY_SIZING_FIT(0, 10000);
        decl.layout.sizing.height = CLAY_SIZING_FIT(0, 10000);
    }

    if (e.has<core::components::UI::Container>()) {
        auto &cont = e.get<core::components::UI::Container>();

        decl.layout.childGap = { cont.gap };
    }

    if (e.has<core::components::UI::Element>()) {
        auto &type = e.get<core::components::UI::Element>();

        if (type == core::components::UI::Element::HBOX) {
            decl.layout.layoutDirection = CLAY_LEFT_TO_RIGHT;
        }
        if (type == core::components::UI::Element::VBOX) {
            decl.layout.layoutDirection = CLAY_TOP_TO_BOTTOM;
        }
        if (type == core::components::UI::Element::BUTTON) {
            spdlog::debug("Declaring button {}", e.name().c_str());
            bool isHover = Clay_PointerOver(decl.id);
            if (isHover) {
                decl.backgroundColor = { 200, 200, 200, 255 };
            }
        }
    }

    if (e.has<core::components::UI::Text>()) {
        decl.layout.sizing.width = CLAY_SIZING_FIT(0, 10000);
        decl.layout.sizing.height = CLAY_SIZING_FIT(0, 10000);
    }

    CLAY(decl)
    {
        if (e.has<core::components::UI::Text>()) {
            auto &txt = e.get_mut<core::components::UI::Text>();

            Clay_String str{ false, (int32_t)txt.content.size(), txt.content.c_str() };
            CLAY_TEXT(str, &txt.text_config);
        }
        e.children([this](flecs::entity child) { declareEntityUi(child); });
    }
}
