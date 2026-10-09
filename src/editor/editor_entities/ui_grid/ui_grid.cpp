#include "ui_grid.hpp"
#include "core/ecs/entities/ui/ui_button/ui_button.hpp"
#include "core/ecs/entities/ui/ui_label/ui_label.hpp"
#include "core/ecs/entity_registry.hpp"
#include "core/types.hpp"
#include "meta/auto_register.hpp"

namespace atmo::core::ecs::entities
{
    void UIGrid::RegisterSystems(flecs::world *) {}

    void UIGrid::initialize()
    {
        UIRect::initialize();
        setComponent<components::UIGrid>({});

        createSignal<int, int>("CellSelected");

        auto &lay = getComponentMutable<core::components::Layout>();
        lay.direction = core::components::Layout::Direction::Vertical;
        lay.child_gap = 2;
        lay.padding = { 4, 4, 4, 4 };
        lay.width.type = SizingAxis::SizingAxisType::GROW;
        lay.height.type = SizingAxis::SizingAxisType::GROW;
        lay.clip.horizontal = true;
        lay.clip.vertical = true;

        getComponentMutable<core::components::UIRect>().color = core::types::Color::TRANSPARENT_COL;
    }

    void UIGrid::setLabels(const std::vector<std::string> &rows, const std::vector<std::string> &cols)
    {
        auto &comp = getComponentMutable<components::UIGrid>();
        comp.row_labels = rows;
        comp.col_labels = cols;
        if (comp.selected_row >= (int)rows.size())
            comp.selected_row = -1;
        if (comp.selected_col >= (int)cols.size())
            comp.selected_col = -1;

        rebuild();
    }

    void UIGrid::setCellBuilder(components::UIGrid::CellBuilder builder)
    {
        getComponentMutable<components::UIGrid>().cell_builder = std::move(builder);
        rebuild();
    }

    void UIGrid::setRowBuilder(components::UIGrid::RowBuilder builder)
    {
        getComponentMutable<components::UIGrid>().row_header_builder = std::move(builder);
        rebuild();
    }

    void UIGrid::setHeaderBuilder(components::UIGrid::HeaderBuilder builder)
    {
        getComponentMutable<components::UIGrid>().header_builder = std::move(builder);
        rebuild();
    }

    void UIGrid::select(int row, int col, bool emit)
    {
        auto &comp = getComponentMutable<components::UIGrid>();
        if (row < -1 || row >= (int)comp.row_labels.size() || col < -1 || col >= (int)comp.col_labels.size()) {
            spdlog::warn("UIGrid: cell ({}, {}) out of range, selection unchanged", row, col);
            return;
        }

        comp.selected_row = row;
        comp.selected_col = col;
        applyHighlight();

        if (emit)
            getSignal<int, int>("CellSelected").emit(row, col);
    }

    int UIGrid::getSelectedRow() const
    {
        return getComponent<components::UIGrid>().selected_row;
    }

    int UIGrid::getSelectedCol() const
    {
        return getComponent<components::UIGrid>().selected_col;
    }

    void UIGrid::refresh()
    {
        rebuild();
    }

    void UIGrid::rebuild()
    {
        for (auto &child : getChildren()) child.destroy();

        buildHeaderRow();
        for (int row = 0; row < (int)getComponent<components::UIGrid>().row_labels.size(); row++)
            buildRow(row);

        applyHighlight();
    }

    void UIGrid::buildHeaderRow()
    {
        const auto comp = getComponent<components::UIGrid>();
        auto handle = p_handle;
        auto line = CreateLine(SizingAxis::SizingAxisType::FIT);
        line->setParent(*this);

        auto corner = core::ecs::EntityRegistry::Create<UI>("Entity::UI");
        auto &corner_lay = corner->getComponentMutable<core::components::Layout>();
        corner_lay.direction = core::components::Layout::Direction::Horizontal;
        SetRowHeaderWidth(corner_lay);
        corner_lay.child_gap = 4;
        corner->setParent(*line);
        if (comp.header_builder)
            comp.header_builder(*corner);

        for (int col = 0; col < (int)comp.col_labels.size(); col++) {
            auto header = CreateLabeledButton(comp.col_labels[col]);
            auto &header_lay = header->getComponentMutable<core::components::Layout>();
            header_lay.width.type = SizingAxis::SizingAxisType::GROW;
            header_lay.height.type = SizingAxis::SizingAxisType::GROW;
            header->setParent(*line);

            header->getSignal<>("Pressed").connect([handle, col]() {
                if (!handle.is_alive())
                    return;
                UIGrid grid(core::ecs::EntityRegistry::GetEntityFromId(handle));
                grid.select(grid.getSelectedRow(), col);
            });
        }
    }

    void UIGrid::buildRow(int row)
    {
        const auto comp = getComponent<components::UIGrid>();
        auto handle = p_handle;
        auto line = CreateLine(SizingAxis::SizingAxisType::GROW);
        line->setParent(*this);

        auto header = core::ecs::EntityRegistry::Create<UI>("Entity::UI");
        auto &header_lay = header->getComponentMutable<core::components::Layout>();
        header_lay.direction = core::components::Layout::Direction::Horizontal;
        SetRowHeaderWidth(header_lay);
        header_lay.child_gap = 4;
        header->setParent(*line);

        auto leading = core::ecs::EntityRegistry::Create<UI>("Entity::UI");
        auto &leading_lay = leading->getComponentMutable<core::components::Layout>();
        leading_lay.direction = core::components::Layout::Direction::Horizontal;
        leading_lay.height.type = SizingAxis::SizingAxisType::GROW;
        leading_lay.child_gap = 4;
        leading->setParent(*header);

        auto box = core::ecs::EntityRegistry::Create<UIRect>("Entity::UI::UIRect");
        auto &box_lay = box->getComponentMutable<core::components::Layout>();
        box->getComponentMutable<core::components::UIRect>().color = core::types::Color::TRANSPARENT_COL;
        box->getComponentMutable<core::components::UIRect>().corner_radius = { 2, 2, 2, 2 };
        box_lay.direction = core::components::Layout::Direction::Horizontal;
        box_lay.width.type = SizingAxis::SizingAxisType::GROW;
        box_lay.height.type = SizingAxis::SizingAxisType::GROW;
        box_lay.child_alignment.vertical = core::components::Layout::ChildAlignment::Center;
        box->setParent(*header);

        auto name = CreateLabeledButton(comp.row_labels[row]);
        auto &name_lay = name->getComponentMutable<core::components::Layout>();
        name_lay.width.type = SizingAxis::SizingAxisType::GROW;
        name_lay.height.type = SizingAxis::SizingAxisType::GROW;
        name_lay.child_alignment.horizontal = core::components::Layout::ChildAlignment::Start;
        name_lay.padding.left = 8;
        name->setParent(*box);

        auto actions = core::ecs::EntityRegistry::Create<UI>("Entity::UI");
        auto &actions_lay = actions->getComponentMutable<core::components::Layout>();
        actions_lay.direction = core::components::Layout::Direction::Horizontal;
        actions_lay.height.type = SizingAxis::SizingAxisType::GROW;
        actions_lay.child_gap = 4;
        actions->setParent(*box);

        if (comp.row_header_builder)
            comp.row_header_builder(*leading, *actions, row);

        name->getSignal<>("Pressed").connect([handle, row]() {
            if (!handle.is_alive())
                return;
            UIGrid grid(core::ecs::EntityRegistry::GetEntityFromId(handle));
            grid.select(row, grid.getSelectedCol());
        });

        for (int col = 0; col < (int)comp.col_labels.size(); col++) {
            auto cell = core::ecs::EntityRegistry::Create<UIButton>("Entity::UI::UIRect::UIButton");
            auto &cell_lay = cell->getComponentMutable<core::components::Layout>();
            cell->getComponentMutable<core::components::UIRect>().corner_radius = { 2, 2, 2, 2 };
            cell_lay.width.type = SizingAxis::SizingAxisType::GROW;
            cell_lay.height.type = SizingAxis::SizingAxisType::GROW;
            cell_lay.padding = { 2, 2, 2, 2 };
            cell->setParent(*line);
            if (comp.cell_builder)
                comp.cell_builder(*cell, row, col);

            cell->getSignal<>("Pressed").connect([handle, row, col]() {
                if (!handle.is_alive())
                    return;
                UIGrid grid(core::ecs::EntityRegistry::GetEntityFromId(handle));
                grid.select(row, col);
            });
        }
    }

    void UIGrid::applyHighlight()
    {
        const auto &comp = getComponent<components::UIGrid>();
        auto lines = getChildren();

        for (int l = 0; l < (int)lines.size(); l++) {
            const int row = l - 1;
            auto items = lines[l].getChildren();

            for (int i = 1; i < (int)items.size(); i++) {
                const int col = i - 1;
                auto &rect = items[i].getComponentMutable<core::components::UIRect>();

                if (row < 0)
                    rect.color = col == comp.selected_col ? GridHeaderHighlightColor : core::types::Color::TRANSPARENT_COL;
                else if (row == comp.selected_row && col == comp.selected_col)
                    rect.color = GridCellSelectedColor;
                else if (row == comp.selected_row || col == comp.selected_col)
                    rect.color = GridHeaderHighlightColor;
                else
                    rect.color = GridCellColor;
            }

            if (row < 0 || items.empty())
                continue;

            auto headerItems = items[0].getChildren();
            if (headerItems.size() < 2)
                continue;
            headerItems[1].getComponentMutable<core::components::UIRect>().color =
                row == comp.selected_row ? GridHeaderHighlightColor : core::types::Color::TRANSPARENT_COL;

            auto boxItems = headerItems[1].getChildren();
            if (boxItems.size() < 2)
                continue;
            boxItems[1].getComponentMutable<core::components::UI>().visible = row == comp.selected_row;
        }
    }

    void UIGrid::SetRowHeaderWidth(core::components::Layout &lay)
    {
        lay.width.type = SizingAxis::SizingAxisType::PERCENT;
        lay.width.size = RowHeaderWidth;
        lay.height.type = SizingAxis::SizingAxisType::GROW;
    }

    std::shared_ptr<UI> UIGrid::CreateLine(SizingAxis::SizingAxisType height)
    {
        auto line = core::ecs::EntityRegistry::Create<UI>("Entity::UI");
        auto &lay = line->getComponentMutable<core::components::Layout>();

        lay.direction = core::components::Layout::Direction::Horizontal;
        lay.width.type = SizingAxis::SizingAxisType::GROW;
        lay.height.type = height;
        lay.child_gap = 2;
        return line;
    }

    std::shared_ptr<UIButton> UIGrid::CreateLabeledButton(const std::string &text)
    {
        auto btn = core::ecs::EntityRegistry::Create<UIButton>("Entity::UI::UIRect::UIButton");
        auto label = core::ecs::EntityRegistry::Create<UILabel>("Entity::UI::UILabel");

        btn->getComponentMutable<core::components::UIRect>().color = core::types::Color::TRANSPARENT_COL;
        btn->getComponentMutable<core::components::UIRect>().corner_radius = { 2, 2, 2, 2 };
        label->setText(text);
        label->setFontSize(12);
        label->getComponentMutable<core::components::UI>().modulate = core::types::Color::BLACK;
        label->setParent(*btn);
        return btn;
    }

    Clay_ElementDeclaration UIGrid::buildDecl()
    {
        return UIRect::buildDecl();
    }

    void UIGrid::draw(ClaySdL3RendererData *) {}
} // namespace atmo::core::ecs::entities

ATMO_REGISTER_ENTITY(entities::UIGrid)
ATMO_REGISTER_COMPONENT(atmo::core::components::UIGrid)
