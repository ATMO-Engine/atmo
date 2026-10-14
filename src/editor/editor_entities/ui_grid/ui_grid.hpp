#pragma once

#include <functional>
#include <string>
#include <vector>
#include "core/ecs/entities/ui/ui_button/ui_button.hpp"
#include "core/ecs/entities/ui/ui_rect/ui_rect.hpp"

namespace atmo::core::components
{
    struct UIGrid {
        using CellBuilder = std::function<void(ecs::entities::Entity &cell, int row, int col)>;
        using RowBuilder = std::function<void(ecs::entities::Entity &leading, ecs::entities::Entity &actions, int row)>;
        using HeaderBuilder = std::function<void(ecs::entities::Entity &corner)>;

        std::vector<std::string> row_labels;
        std::vector<std::string> col_labels;
        int selected_row = -1;
        int selected_col = -1;

        CellBuilder cell_builder;
        RowBuilder row_header_builder;
        HeaderBuilder header_builder;
    };
} // namespace atmo::core::components

template <> struct atmo::meta::ComponentMeta<atmo::core::components::UIGrid> {
    static constexpr const char *name = "Grid";
    static constexpr const char *category = "UI";
    static constexpr auto fields = std::make_tuple(
        atmo::meta::field<&atmo::core::components::UIGrid::selected_row>("selected_row"),
        atmo::meta::field<&atmo::core::components::UIGrid::selected_col>("selected_col"));
};

namespace atmo::core::ecs::entities
{
    using SizingAxis = core::components::Layout::SizingAxis;

    const types::Color GridCellColor("#F3F4F6");
    const types::Color GridHeaderHighlightColor("#DBEAFE");
    const types::Color GridCellSelectedColor("#93C5FD");
    const float RowHeaderWidth = 0.2f;

    class UIGrid : public EntityRegistry::Registrable<UIGrid, UIRect>
    {
    public:
        using EntityRegistry::Registrable<UIGrid, UIRect>::Registrable;

        static void RegisterSystems(flecs::world *world);

        void initialize();

        static constexpr std::string_view LocalName()
        {
            return "UIGrid";
        }

        /**
         * @brief Sets both axes at once and rebuilds the grid
         *
         * The selection is kept on each axis that is still in range, unselected (-1) otherwise.
         */
        void setLabels(const std::vector<std::string> &rows, const std::vector<std::string> &cols);
        void setCellBuilder(components::UIGrid::CellBuilder builder);
        void setRowBuilder(components::UIGrid::RowBuilder builder);

        /**
         * @brief Fills the top left corner of the grid (above the row headers), called on every rebuild
         */
        void setHeaderBuilder(components::UIGrid::HeaderBuilder builder);

        /**
         * @brief Highlights a cell and emits "CellSelected"
         *
         * @param row Index of the row, -1 to unselect the row
         * @param col Index of the column, -1 to unselect the column
         * @param emit false to only update the highlight, without emitting the signal
         */
        void select(int row, int col, bool emit = true);
        int getSelectedRow() const;
        int getSelectedCol() const;

        /**
         * @brief Rebuilds every line, to call when what the builders display changed
         */
        void refresh();

        Clay_ElementDeclaration buildDecl() override;
        void draw(ClaySdL3RendererData *data) override;

    private:
        static void SetRowHeaderWidth(core::components::Layout &lay);
        static std::shared_ptr<UI> CreateLine(core::components::Layout::SizingAxis::SizingAxisType height);
        static std::shared_ptr<UIButton> CreateLabeledButton(const std::string &text);

        void rebuild();
        void buildHeaderRow();
        void buildRow(int row);
        void applyHighlight();
    };
} // namespace atmo::core::ecs::entities
