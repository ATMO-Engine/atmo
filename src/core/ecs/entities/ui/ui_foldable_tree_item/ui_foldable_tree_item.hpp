#pragma once

#include "core/ecs/entities/ui/ui_button/ui_button.hpp"

namespace atmo::core::components
{
    struct UIFoldableTreeItem {
        bool open = true;
    };
} // namespace atmo::core::components

template <> struct atmo::meta::ComponentMeta<atmo::core::components::UIFoldableTreeItem> {
    static constexpr const char *name = "FoldableTreeItem";
    static constexpr const char *category = "UI";
    static constexpr auto fields = std::make_tuple(atmo::meta::field<&atmo::core::components::UIFoldableTreeItem::open>("open"));
};

namespace atmo::core::ecs::entities
{
    class UIFoldableTreeItem : public EntityRegistry::Registrable<UIFoldableTreeItem, UIButton>
    {
    public:
        using EntityRegistry::Registrable<UIFoldableTreeItem, UIButton>::Registrable;

        static void RegisterSystems(flecs::world *world);

        void initialize();

        static constexpr std::string_view LocalName()
        {
            return "UIFoldableTreeItem";
        }

        Clay_ElementDeclaration buildDecl() override;
        void draw(ClaySdL3RendererData *data) override;
    };
} // namespace atmo::core::ecs::entities
