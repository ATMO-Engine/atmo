#include "ui.hpp"
#include "common/math.hpp"
#include "core/ecs/entities/ui/ui_layout.hpp"
#include "core/ecs/entities/window/window.hpp"
#include "meta/auto_register.hpp"

#include "clay.h"
#include "spdlog/spdlog.h"

namespace atmo::core::ecs::entities
{
    void UI::RegisterSystems(flecs::world *world) {}

    void UI::initialize()
    {
        Entity::initialize();

        setComponent<components::UI>({ .element_id = getIdForEntity() });

        setComponent<components::Layout>({});
    }

    Clay_ElementId UI::getIdForEntity()
    {
        std::string path = std::format("#{}", getID());
        Clay_String s{ false, static_cast<std::int32_t>(path.size()), path.c_str() };
        return Clay_GetElementId(s);
    }

    Clay_ElementDeclaration UI::buildDecl()
    {
        Clay_ElementDeclaration d{};
        d.id = getComponent<components::UI>().element_id;

        if (hasComponent<components::Layout>()) {
            const auto &layout = getComponentMutable<components::Layout>();
            configureSizingAxis(d.layout.sizing.width, layout.width);
            configureSizingAxis(d.layout.sizing.height, layout.height);

            if (layout.aspect_ratio.width > 0.0f && layout.aspect_ratio.height > 0.0f)
                d.aspectRatio = { .aspectRatio = layout.aspect_ratio.width / layout.aspect_ratio.height };

            d.layout.padding.left = layout.padding.left;
            d.layout.padding.right = layout.padding.right;
            d.layout.padding.bottom = layout.padding.bottom;
            d.layout.padding.top = layout.padding.top;

            if (layout.clip.horizontal || layout.clip.vertical)
                d.clip = { .horizontal = layout.clip.horizontal, .vertical = layout.clip.vertical, .childOffset = Clay_GetScrollOffset() };

            d.layout.childAlignment = {
                layout.child_alignment.horizontal == components::Layout::ChildAlignment::Start        ? CLAY_ALIGN_X_LEFT
                    : layout.child_alignment.horizontal == components::Layout::ChildAlignment::Center ? CLAY_ALIGN_X_CENTER
                                                                                                      : CLAY_ALIGN_X_RIGHT,
                layout.child_alignment.vertical == components::Layout::ChildAlignment::Start        ? CLAY_ALIGN_Y_TOP
                    : layout.child_alignment.vertical == components::Layout::ChildAlignment::Center ? CLAY_ALIGN_Y_CENTER
                                                                                                    : CLAY_ALIGN_Y_BOTTOM,
            };

            d.layout.childGap = layout.child_gap;
            d.layout.layoutDirection = layout.direction == components::Layout::Direction::Horizontal ? CLAY_LEFT_TO_RIGHT : CLAY_TOP_TO_BOTTOM;

            if (layout.floating)
                d.floating.attachTo = CLAY_ATTACH_TO_ROOT;

            d.floating.zIndex = layout.z_index;
        }

        return d;
    }

    void UI::internalDraw(ClaySdL3RendererData *data)
    {
        std::vector<Entity> children = getChildren();
        bool has_ui_child = false;

        for (auto &entity : children)
            if (entity.hasComponent<components::UI>())
                has_ui_child = true;

        Clay__OpenElement();
        Clay__ConfigureOpenElement(buildDecl());

        draw(data);

        if (has_ui_child) {
            for (auto child : children) {
                if (child.hasComponent<components::UI>())
                    if (child.getComponentMutable<components::UI>().visible == false)
                        continue;
                auto wrapped = EntityRegistry::Wrap(child);
                if (auto *ui = dynamic_cast<entities::UI *>(wrapped.get()))
                    ui->internalDraw(data);
            }
        }

        Clay__CloseElement();
    }

    void UI::draw(ClaySdL3RendererData *data) {}

    void UI::configureSizingAxis(Clay_SizingAxis &sizing, const components::Layout::SizingAxis &axis)
    {
        switch (axis.type) {
            case components::Layout::SizingAxis::SizingAxisType::FIT:
            case components::Layout::SizingAxis::SizingAxisType::GROW:
            case components::Layout::SizingAxis::SizingAxisType::FIXED:
                sizing.size.minMax.min = std::get<components::Layout::SizingAxis::MinMax>(axis.size).min;
                sizing.size.minMax.max = std::get<components::Layout::SizingAxis::MinMax>(axis.size).max;
                break;
            case components::Layout::SizingAxis::SizingAxisType::PERCENT:
                sizing.size.percent = common::math::Clamp(std::get<float>(axis.size), 0.0f, 1.0f);
                break;
        }

        switch (axis.type) {
            case components::Layout::SizingAxis::SizingAxisType::FIT:
                sizing.type = CLAY__SIZING_TYPE_FIT;
                break;
            case components::Layout::SizingAxis::SizingAxisType::GROW:
                sizing.type = CLAY__SIZING_TYPE_GROW;
                break;
            case components::Layout::SizingAxis::SizingAxisType::FIXED:
                sizing.type = CLAY__SIZING_TYPE_FIXED;
                break;
            case components::Layout::SizingAxis::SizingAxisType::PERCENT:
                sizing.type = CLAY__SIZING_TYPE_PERCENT;
                break;
        }
    }

    std::shared_ptr<entities::Window> UI::getWindow() const
    {
        flecs::entity current = p_handle;

        while (current.is_valid()) {
            if (current.has<components::Window>()) {
                return std::make_shared<entities::Window>(current);
            }
            current = current.parent();
        }

        return nullptr;
    }
} // namespace atmo::core::ecs::entities

ATMO_REGISTER_ENTITY(entities::UI);
ATMO_REGISTER_COMPONENT(atmo::core::components::UI)
