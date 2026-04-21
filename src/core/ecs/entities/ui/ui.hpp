#pragma once

#include <string>

#include "clay.h"
#include "core/ecs/entities/ui/ui_layout.hpp"
#include "core/ecs/entity_registry.hpp"
#include "core/types.hpp"
#include "impl/clay_types.hpp"
#include "meta/meta.hpp"


namespace atmo::core::components
{
    struct UI {
        bool visible{ true };
        types::Color modulate{ types::Color::WHITE };
        Clay_ElementId element_id;
    };
} // namespace atmo::core::components

template <> struct atmo::meta::ComponentMeta<atmo::core::components::UI> {
    static constexpr const char *name = "UI";
    static constexpr const char *category = "UI";
    static constexpr auto fields = std::make_tuple(
        atmo::meta::field<&atmo::core::components::UI::UI::visible>("visible"),
        atmo::meta::field<&atmo::core::components::UI::UI::modulate>("modulate").withWidget("color_picker"));
};

namespace atmo::core::ecs::entities
{
    class UI : public EntityRegistry::Registrable<UI, Entity>
    {
    public:
        using EntityRegistry::Registrable<UI, Entity>::Registrable;

        static void RegisterSystems(flecs::world *world);

        void initialize();

        static constexpr std::string_view LocalName()
        {
            return "UI";
        }

        void internalDraw(ClaySdL3RendererData *data);
        virtual void draw(ClaySdL3RendererData *data);

    protected:
        virtual Clay_ElementDeclaration buildDecl();

    private:
        Clay_ElementId getIdForEntity();
        void configureSizingAxis(Clay_SizingAxis &sizing, const components::Layout::SizingAxis &axis);
    };
} // namespace atmo::core::ecs::entities
