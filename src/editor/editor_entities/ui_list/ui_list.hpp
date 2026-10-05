#pragma once

#include <string>
#include <vector>
#include "core/ecs/entities/ui/ui_layout.hpp"
#include "core/ecs/entities/ui/ui_rect/ui_rect.hpp"

namespace atmo::core::components
{
    struct UIList {
        std::vector<std::string> items;
        int selected = -1;
    };
} // namespace atmo::core::components

template <> struct atmo::meta::ComponentMeta<atmo::core::components::UIList> {
    static constexpr const char *name = "List";
    static constexpr const char *category = "UI";
    static constexpr auto fields = std::make_tuple(
        atmo::meta::field<&atmo::core::components::UIList::selected>("selected"));
};

namespace atmo::core::ecs::entities
{
    const types::Color HighlightColor("#DBEAFE");

    class UIList : public EntityRegistry::Registrable<UIList, UIRect>
    {
    public:
        using EntityRegistry::Registrable<UIList, UIRect>::Registrable;

        static void RegisterSystems(flecs::world *world);

        void initialize();

        static constexpr std::string_view LocalName()
        {
            return "UIList";
        }

        void setDirection(core::components::Layout::Direction direction);

        void setItems(const std::vector<std::string> &items);
        void addItem(const std::string &item);
        void removeItem(int index);
        void clear();

        /**
         * @brief Highlights an item and emits "ItemSelected"
         *
         * @param index Index of the item, -1 to unselect everything
         * @param emit false to only update the highlight, without emitting the signal
         */
        void select(int index, bool emit = true);
        int getSelected() const;
        std::string getSelectedItem() const;

        Clay_ElementDeclaration buildDecl() override;
        void draw(ClaySdL3RendererData *data) override;

    private:
        void rebuild();
        void applyItemLayout(Entity &item) const;
    };
} // namespace atmo::core::ecs::entities
