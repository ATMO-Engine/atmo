#include "ui_label.hpp"
#include "SDL3/SDL_error.h"
#include "SDL3_ttf/SDL_ttf.h"
#include "clay.h"
#include "core/ecs/entities/ui/ui.hpp"
#include "core/ecs/entities/window/window.hpp"
#include "core/resource/resource_manager.hpp"
#include "core/resource/resource_ref.hpp"
#include "core/types.hpp"
#include "meta/auto_register.hpp"
#include "spdlog/spdlog.h"

namespace atmo::core::ecs::entities
{
    void UILabel::RegisterSystems(flecs::world *world)
    {
        world->observer<components::UILabel>("UILabel_remove").event(flecs::OnRemove).each([](flecs::entity e, components::UILabel &label) {
            if (label.font_path.empty())
                return;

            resource::ResourceRef<TTF_Font> res = resource::ResourceManager::GetInstance().getResource<TTF_Font>(label.font_handle.assetId);

            res.unpin();

            TTF_DestroyText(label.ttf_text);
        });
    }

    void UILabel::initialize()
    {
        UI::initialize();

        setComponent<components::UILabel>({});
    }

    void UILabel::setFontPath(const std::string &path)
    {
        auto label = p_handle.get_ref<components::UILabel>();
        label->font_path = path;

        if (label->font_path.empty())
            return;

        label->font_handle = resource::Handle<TTF_Font>{ .assetId = label->font_path };

        resource::ResourceRef<TTF_Font> res = resource::ResourceManager::GetInstance().getResource<TTF_Font>(label->font_handle.assetId);

        res.pin();

        label->ttf_text = TTF_CreateText(nullptr, res.get().get(), label->text.c_str(), label->text.size());
    }

    std::string_view UILabel::getFontPath() const noexcept
    {
        auto label = p_handle.get_ref<components::UILabel>();
        return label->font_path;
    }

    void UILabel::setText(const std::string &text)
    {
        auto &label = getComponentMutable<components::UILabel>();
        label.text = text;

        if (label.ttf_text) {
            if (!TTF_SetTextString(label.ttf_text, text.c_str(), text.size()))
                spdlog::error("setText on UILabel failed: {}", SDL_GetError());
        }
    }

    std::string_view UILabel::getText()
    {
        const auto &label = getComponent<components::UILabel>();
        return label.text;
    }

    void UILabel::draw(ClaySdL3RendererData *data)
    {
        const auto &ui = getComponent<components::UI>();
        const auto &label = getComponent<components::UILabel>();

        TTF_SetTextEngine(label.ttf_text, data->text_engine);

        Clay_String str = Clay_String{ .isStaticallyAllocated = false, .length = static_cast<int32_t>(label.text.size()), .chars = label.text.data() };
        CLAY_TEXT(
            str,
            CLAY_TEXT_CONFIG(
                { .userData = (void *)label.ttf_text,
                  .textColor = (types::Color::WHITE * ui.modulate).toFloat<Clay_Color>(255),
                  .fontSize = static_cast<uint16_t>(label.font_size),
                  .letterSpacing = label.letter_spacing,
                  .lineHeight = label.line_height,
                  .wrapMode = static_cast<Clay_TextElementConfigWrapMode>(label.wrap_mode),
                  .textAlignment = static_cast<Clay_TextAlignment>(label.text_alignment) }));
    }
} // namespace atmo::core::ecs::entities

ATMO_REGISTER_ENTITY(entities::UILabel);
ATMO_REGISTER_COMPONENT(atmo::core::components::UILabel)
