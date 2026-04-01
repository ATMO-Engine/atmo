#include "ui_rect.hpp"
#include "meta/auto_register.hpp"
#include "spdlog/spdlog.h"

namespace atmo::core::ecs::entities
{
    void UIRect::RegisterSystems(flecs::world *world) {}

    void UIRect::initialize()
    {
        UI::initialize();

        setComponent<components::UIRect>({});
    }

    Clay_ElementDeclaration UIRect::buildDecl()
    {
        Clay_ElementDeclaration d = UI::buildDecl();

        const auto &rect = getComponentMutable<components::UIRect>();

        d.border.color = { static_cast<float>(rect.border.color.r),
                           static_cast<float>(rect.border.color.g),
                           static_cast<float>(rect.border.color.b),
                           static_cast<float>(rect.border.color.a) };

        d.border.width = { rect.border.left, rect.border.right, rect.border.top, rect.border.bottom, rect.border.between_children };

        d.backgroundColor = {
            static_cast<float>(rect.color.r), static_cast<float>(rect.color.g), static_cast<float>(rect.color.b), static_cast<float>(rect.color.a)
        };
        d.cornerRadius = { rect.corner_radius.top_left, rect.corner_radius.top_right, rect.corner_radius.bottom_left, rect.corner_radius.bottom_right };

        return d;
    }

    void UIRect::draw()
    {
        CLAY(buildDecl())
        {
            for (auto child : getChildren()) {
                if (child.hasComponent<components::UI>()) {
                    auto wrapped = EntityRegistry::Wrap(child);
                    if (auto *ui = dynamic_cast<entities::UI *>(wrapped.get()))
                        ui->draw();
                }
            }
        }
    }
} // namespace atmo::core::ecs::entities

ATMO_REGISTER_ENTITY(entities::UIRect);
ATMO_REGISTER_COMPONENT(atmo::core::components::UIRect)
