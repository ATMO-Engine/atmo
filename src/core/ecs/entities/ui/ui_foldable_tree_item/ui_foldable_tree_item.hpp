#pragma once

#include "core/ecs/entities/ui/ui_button/ui_button.hpp"
#include "core/ecs/entities/ui/ui_label/ui_label.hpp"
#include "core/ecs/entities/ui/ui_rect/ui_rect.hpp"

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
    class UIFoldableTreeItem : public EntityRegistry::Registrable<UIFoldableTreeItem, UIRect>
    {
    public:
        using EntityRegistry::Registrable<UIFoldableTreeItem, UIRect>::Registrable;

        static void RegisterSystems(flecs::world *world);

        void initialize();

        static constexpr std::string_view LocalName()
        {
            return "UIFoldableTreeItem";
        }

        UIButton getTitleButton() const;
        UILabel getTitleLabel() const;
        UIRect getChildContainer() const;

        Clay_ElementDeclaration buildDecl() override;
        void draw(ClaySdL3RendererData *data) override;
    };
} // namespace atmo::core::ecs::entities
