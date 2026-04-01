#pragma once

#include <string>

#include "clay.h"
#include "core/ecs/entities/ui/ui_layout.hpp"
#include "core/ecs/entity_registry.hpp"
#include "core/types.hpp"
#include "meta/meta.hpp"


namespace atmo::core::components
{
    struct UI {
        bool visible{ true };
        types::ColorRGBA modulate{ 1.0f, 1.0f, 1.0f, 1.0f };
        types::ColorRGBA self_modulate{ 1.0f, 1.0f, 1.0f, 1.0f };
        Clay_ElementId element_id;
    };
} // namespace atmo::core::components

template <> struct atmo::meta::ComponentMeta<atmo::core::components::UI> {
    static constexpr const char *name = "UI";
    static constexpr const char *category = "UI";
    static constexpr auto fields = std::make_tuple(
        atmo::meta::field<&atmo::core::components::UI::UI::visible>("visible"),
        atmo::meta::field<&atmo::core::components::UI::UI::modulate>("modulate").withWidget("color_picker"),
        atmo::meta::field<&atmo::core::components::UI::UI::self_modulate>("self_modulate").withWidget("color_picker"));
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

        virtual void draw();

    protected:
        virtual Clay_ElementDeclaration buildDecl();

    private:
        Clay_ElementId getIdForEntity();
        void configureSizingAxis(Clay_SizingAxis &sizing, const components::Layout::SizingAxis &axis);
    };
} // namespace atmo::core::ecs::entities
