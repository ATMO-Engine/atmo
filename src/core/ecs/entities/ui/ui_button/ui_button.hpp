#pragma once

#include <string>
#include <sys/_types/_u_int32_t.h>
#include "core/ecs/entities/ui/ui_rect/ui_rect.hpp"
#include "flecs/addons/cpp/mixins/query/impl.hpp"

namespace atmo::core::components
{
    struct UIButton {
        bool is_hovered = false;
        bool is_pressed = false;
        bool toggle = false;
        int group = 0;
    };
} // namespace atmo::core::components

template <> struct atmo::meta::ComponentMeta<atmo::core::components::UIButton> {
    static constexpr const char *name = "Button";
    static constexpr const char *category = "UI";
    static constexpr auto fields = std::make_tuple();
};

namespace atmo::core::ecs::entities
{
    class UIButton : public EntityRegistry::Registrable<UIButton, UIRect>
    {
    public:
        using EntityRegistry::Registrable<UIButton, UIRect>::Registrable;

        static void RegisterSystems(flecs::world *world);
        static void Unregister(flecs::world *world);

        void initialize();

        static constexpr std::string_view LocalName()
        {
            return "UIButton";
        }

        Clay_ElementDeclaration buildDecl() override;
        void draw(ClaySdL3RendererData *data) override;

    private:
        static flecs::query<components::UIButton, components::UIRect> *m_query;
    };
} // namespace atmo::core::ecs::entities
