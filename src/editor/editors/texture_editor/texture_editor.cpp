#include "texture_editor.hpp"
#include "core/ecs/entities/ui/ui_button/ui_button.hpp"
#include "core/ecs/entities/ui/ui_input/ui_number_input/ui_number_input.hpp"
#include "core/ecs/entities/ui/ui_input/ui_text_input/ui_text_input.hpp"
#include "core/ecs/entities/ui/ui_label/ui_label.hpp"
#include "core/ecs/entities/ui/ui_layout.hpp"
#include "core/ecs/entities/ui/ui_rect/ui_rect.hpp"
#include "core/ecs/entities/ui/ui_slider/ui_slider.hpp"
#include "core/types.hpp"
#include "editor/editor_entities/ui_color_picker/ui_color_picker.hpp"
#include "editor/editor_entities/ui_drawing_canvas/ui_drawing_canvas.hpp"
#include "editor/editor_entities/ui_file_explorer/ui_file_explorer.hpp"
#include "editor/editor_registry.hpp"

namespace atmo::editor
{
    void TextureEditor::init(atmo::core::ecs::entities::UI &container)
    {
        auto texture_editor_container = core::ecs::EntityRegistry::Create<core::ecs::entities::UI>("Entity::UI");
        auto &texture_editor_container_layout = texture_editor_container->getComponentMutable<core::components::Layout>();
        texture_editor_container_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        texture_editor_container_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        texture_editor_container_layout.padding = { 16, 16, 8, 16 };
        texture_editor_container_layout.child_alignment.vertical = core::components::Layout::ChildAlignment::Start;
        texture_editor_container->setParent(container);

        auto option_panel = core::ecs::EntityRegistry::Create<core::ecs::entities::UIRect>("Entity::UI::UIRect");
        auto &option_panel_rect = option_panel->getComponentMutable<core::components::UIRect>();
        option_panel_rect.color.a = 0;
        auto &option_panel_layout = option_panel->getComponentMutable<core::components::Layout>();
        option_panel_layout.direction = core::components::Layout::Direction::Vertical;
        option_panel_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::PERCENT;
        option_panel_layout.width.size = 0.2f;
        option_panel_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        option_panel_layout.child_gap = 16;
        option_panel->setParent(*texture_editor_container);

        auto texture_editor_panel = core::ecs::EntityRegistry::Create<core::ecs::entities::UIRect>("Entity::UI::UIRect");
        auto &texture_editor_panel_rect = texture_editor_panel->getComponentMutable<core::components::UIRect>();
        texture_editor_panel_rect.corner_radius.top_left = 4.0f;
        texture_editor_panel_rect.corner_radius.top_right = 4.0f;
        texture_editor_panel_rect.corner_radius.bottom_left = 4.0f;
        texture_editor_panel_rect.corner_radius.bottom_right = 4.0f;
        texture_editor_panel_rect.border.color = core::types::Color("#dbdbdb");
        texture_editor_panel_rect.color = core::types::Color("#dbdbdb");
        auto &texture_editor_panel_layout = texture_editor_panel->getComponentMutable<core::components::Layout>();
        texture_editor_panel_layout.direction = core::components::Layout::Direction::Vertical;
        texture_editor_panel_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        texture_editor_panel_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::PERCENT;
        texture_editor_panel_layout.height.size = 0.33f;
        texture_editor_panel_layout.padding.left = 16;
        texture_editor_panel_layout.padding.right = 16;
        texture_editor_panel_layout.padding.top = 16;
        texture_editor_panel_layout.padding.bottom = 16;
        texture_editor_panel_layout.child_gap = 16;
        texture_editor_panel->setParent(*option_panel);

        auto size_comp_panel = core::ecs::EntityRegistry::Create<core::ecs::entities::UIRect>("Entity::UI::UIRect");
        auto &size_comp_panel_rect = size_comp_panel->getComponentMutable<core::components::UIRect>();
        size_comp_panel_rect.corner_radius.top_left = 5.0f;
        size_comp_panel_rect.corner_radius.top_right = 5.0f;
        size_comp_panel_rect.corner_radius.bottom_left = 5.0f;
        size_comp_panel_rect.corner_radius.bottom_right = 5.0f;
        size_comp_panel_rect.border.color = core::types::Color("#7d7d7d");
        size_comp_panel_rect.color = core::types::Color("#7d7d7d");
        auto &size_comp_panel_layout = size_comp_panel->getComponentMutable<core::components::Layout>();
        size_comp_panel_layout.direction = core::components::Layout::Direction::Horizontal;
        size_comp_panel_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        size_comp_panel_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::FIT;
        size_comp_panel_layout.padding.left = 16;
        size_comp_panel_layout.padding.right = 16;
        size_comp_panel_layout.padding.top = 16;
        size_comp_panel_layout.padding.bottom = 16;
        size_comp_panel_layout.child_gap = 0;
        size_comp_panel->setParent(*texture_editor_panel);

        auto sizeLabel = core::ecs::EntityRegistry::Create<core::ecs::entities::UILabel>("Entity::UI::UILabel");
        sizeLabel->setText("Size");
        sizeLabel->setFontSize(16);
        sizeLabel->setParent(*size_comp_panel);

        auto sizeSlider = core::ecs::EntityRegistry::Create<core::ecs::entities::UISlider>("Entity::UI::UIRect::UISlider");
        sizeSlider->setType(core::components::UISlider::SliderType::Int, 1, 10);
        sizeSlider->setParent(*size_comp_panel);

        auto sizeNumberInput = core::ecs::EntityRegistry::Create<core::ecs::entities::UINumberInput>("Entity::UI::UIInput::UINumberInput");
        auto &size_input_entity_comp = sizeNumberInput->getComponentMutable<core::components::UIInput>();
        size_input_entity_comp.input_type = atmo::core::components::UIInput::InputType::Int;
        sizeNumberInput->setParent(*size_comp_panel);


        auto width_comp_panel = core::ecs::EntityRegistry::Create<core::ecs::entities::UIRect>("Entity::UI::UIRect");
        auto &width_comp_panel_rect = width_comp_panel->getComponentMutable<core::components::UIRect>();
        width_comp_panel_rect.corner_radius.top_left = 5.0f;
        width_comp_panel_rect.corner_radius.top_right = 5.0f;
        width_comp_panel_rect.corner_radius.bottom_left = 5.0f;
        width_comp_panel_rect.corner_radius.bottom_right = 5.0f;
        width_comp_panel_rect.border.color = core::types::Color("#7d7d7d");
        width_comp_panel_rect.color = core::types::Color("#7d7d7d");
        auto &width_comp_panel_layout = width_comp_panel->getComponentMutable<core::components::Layout>();
        width_comp_panel_layout.direction = core::components::Layout::Direction::Horizontal;
        width_comp_panel_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        width_comp_panel_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::FIT;
        width_comp_panel_layout.padding.left = 16;
        width_comp_panel_layout.padding.right = 16;
        width_comp_panel_layout.padding.top = 16;
        width_comp_panel_layout.padding.bottom = 16;
        width_comp_panel_layout.child_gap = 0;
        width_comp_panel->setParent(*texture_editor_panel);

        auto widthLabel = core::ecs::EntityRegistry::Create<core::ecs::entities::UILabel>("Entity::UI::UILabel");
        widthLabel->setText("Width");
        widthLabel->setFontSize(16);
        widthLabel->setParent(*width_comp_panel);

        auto widthNumberInput = core::ecs::EntityRegistry::Create<core::ecs::entities::UINumberInput>("Entity::UI::UIInput::UINumberInput");
        auto &width_input_entity_comp = widthNumberInput->getComponentMutable<core::components::UIInput>();
        width_input_entity_comp.input_type = atmo::core::components::UIInput::InputType::Int;
        widthNumberInput->setParent(*width_comp_panel);


        auto height_comp_panel = core::ecs::EntityRegistry::Create<core::ecs::entities::UIRect>("Entity::UI::UIRect");
        auto &height_comp_panel_rect = height_comp_panel->getComponentMutable<core::components::UIRect>();
        height_comp_panel_rect.corner_radius.top_left = 5.0f;
        height_comp_panel_rect.corner_radius.top_right = 5.0f;
        height_comp_panel_rect.corner_radius.bottom_left = 5.0f;
        height_comp_panel_rect.corner_radius.bottom_right = 5.0f;
        height_comp_panel_rect.border.color = core::types::Color("#7d7d7d");
        height_comp_panel_rect.color = core::types::Color("#7d7d7d");
        auto &height_comp_panel_layout = height_comp_panel->getComponentMutable<core::components::Layout>();
        height_comp_panel_layout.direction = core::components::Layout::Direction::Horizontal;
        height_comp_panel_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        height_comp_panel_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::FIT;
        height_comp_panel_layout.padding.left = 16;
        height_comp_panel_layout.padding.right = 16;
        height_comp_panel_layout.padding.top = 16;
        height_comp_panel_layout.padding.bottom = 16;
        height_comp_panel_layout.child_gap = 0;
        height_comp_panel->setParent(*texture_editor_panel);

        auto heightLabel = core::ecs::EntityRegistry::Create<core::ecs::entities::UILabel>("Entity::UI::UILabel");
        heightLabel->setText("Height");
        heightLabel->setFontSize(16);
        heightLabel->setParent(*height_comp_panel);

        auto heightNumberInput = core::ecs::EntityRegistry::Create<core::ecs::entities::UINumberInput>("Entity::UI::UIInput::UINumberInput");
        auto &height_input_entity_comp = heightNumberInput->getComponentMutable<core::components::UIInput>();
        height_input_entity_comp.input_type = atmo::core::components::UIInput::InputType::Int;
        heightNumberInput->setParent(*height_comp_panel);


        auto saveBtn = core::ecs::EntityRegistry::Create<core::ecs::entities::UIButton>("Entity::UI::UIRect::UIButton");
        ((core::ecs::entities::UILabel)saveBtn->getChildren()[0]).setText("Save");
        auto &saveBtn_rect = saveBtn->getComponentMutable<core::components::UIRect>();
        saveBtn_rect.border.color = core::types::Color::BLACK;
        saveBtn_rect.color = core::types::Color::WHITE;
        auto &saveBtn_layout = saveBtn->getComponentMutable<core::components::Layout>();
        saveBtn_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::PERCENT;
        saveBtn_layout.width.size = 0.30f;
        saveBtn_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::PERCENT;
        saveBtn_layout.height.size = 0.05f;
        saveBtn->setParent(*texture_editor_panel);

        auto previewBtn = core::ecs::EntityRegistry::Create<core::ecs::entities::UIButton>("Entity::UI::UIRect::UIButton");
        ((core::ecs::entities::UILabel)previewBtn->getChildren()[0]).setText("Preview");
        auto &previewBtn_rect = previewBtn->getComponentMutable<core::components::UIRect>();
        previewBtn_rect.border.color = core::types::Color::BLACK;
        previewBtn_rect.color = core::types::Color::WHITE;
        auto &previewBtn_layout = previewBtn->getComponentMutable<core::components::Layout>();
        previewBtn_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::PERCENT;
        previewBtn_layout.width.size = 0.30f;
        previewBtn_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::PERCENT;
        previewBtn_layout.height.size = 0.05f;
        previewBtn->setParent(*texture_editor_panel);


        auto pencilContainer = core::ecs::EntityRegistry::Create<core::ecs::entities::UIRect>("Entity::UI::UIRect");
        auto &btnContainer_rect = pencilContainer->getComponentMutable<core::components::UIRect>();
        btnContainer_rect.color.a = 0;
        auto &pencilContainer_layout = pencilContainer->getComponentMutable<core::components::Layout>();
        pencilContainer_layout.direction = core::components::Layout::Direction::Horizontal;
        pencilContainer_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        pencilContainer_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::PERCENT;
        pencilContainer_layout.height.size = 0.05f;
        pencilContainer_layout.child_gap = 16;
        pencilContainer->setParent(*texture_editor_panel);

        auto pencilBtn = core::ecs::EntityRegistry::Create<core::ecs::entities::UIButton>("Entity::UI::UIRect::UIButton");
        ((core::ecs::entities::UILabel)pencilBtn->getChildren()[0]).setText("Draw");
        auto &pencilBtn_rect = pencilBtn->getComponentMutable<core::components::UIRect>();
        pencilBtn_rect.border.color = core::types::Color::BLACK;
        pencilBtn_rect.color = core::types::Color::WHITE;
        auto &pencilBtn_layout = pencilBtn->getComponentMutable<core::components::Layout>();
        pencilBtn_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::PERCENT;
        pencilBtn_layout.width.size = 0.30f;
        pencilBtn_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        pencilBtn->setParent(*pencilContainer);

        auto eraserBtn = core::ecs::EntityRegistry::Create<core::ecs::entities::UIButton>("Entity::UI::UIRect::UIButton");
        ((core::ecs::entities::UILabel)eraserBtn->getChildren()[0]).setText("Erase");
        auto &eraserBtn_rect = eraserBtn->getComponentMutable<core::components::UIRect>();
        eraserBtn_rect.border.color = core::types::Color::BLACK;
        eraserBtn_rect.color = core::types::Color::WHITE;
        auto &eraserBtn_layout = eraserBtn->getComponentMutable<core::components::Layout>();
        eraserBtn_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::PERCENT;
        eraserBtn_layout.width.size = 0.30f;
        eraserBtn_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        eraserBtn->setParent(*pencilContainer);


        auto fileExplorerContainer = core::ecs::EntityRegistry::Create<core::ecs::entities::UIRect>("Entity::UI::UIRect");
        auto &fileExplorerContainer_rect = fileExplorerContainer->getComponentMutable<core::components::UIRect>();
        fileExplorerContainer_rect.border.color = core::types::Color("#dbdbdb");
        fileExplorerContainer_rect.color = core::types::Color("#dbdbdb");
        fileExplorerContainer_rect.corner_radius.top_left = 4.0f;
        fileExplorerContainer_rect.corner_radius.top_right = 4.0f;
        fileExplorerContainer_rect.corner_radius.bottom_left = 4.0f;
        fileExplorerContainer_rect.corner_radius.bottom_right = 4.0f;
        auto &fileExplorerContainer_layout = fileExplorerContainer->getComponentMutable<core::components::Layout>();
        fileExplorerContainer_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        fileExplorerContainer_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::PERCENT;
        fileExplorerContainer_layout.height.size = 0.33f;
        fileExplorerContainer_layout.padding.left = 16;
        fileExplorerContainer_layout.padding.right = 16;
        fileExplorerContainer_layout.padding.top = 16;
        fileExplorerContainer_layout.padding.bottom = 16;
        fileExplorerContainer->setParent(*option_panel);

        auto fileExplorer = core::ecs::EntityRegistry::Create<core::ecs::entities::UIFileExplorer>("Entity::UI::UIRect::UIFileExplorer");
        fileExplorer->setRootPath(std::filesystem::current_path().string());
        fileExplorer->setParent(*fileExplorerContainer);


        auto colorPickContainer = core::ecs::EntityRegistry::Create<core::ecs::entities::UIRect>("Entity::UI::UIRect");
        auto &colorPickContaine_rect = colorPickContainer->getComponentMutable<core::components::UIRect>();
        colorPickContaine_rect.border.color = core::types::Color("#dbdbdb");
        colorPickContaine_rect.color = core::types::Color("#dbdbdb");
        colorPickContaine_rect.corner_radius.top_left = 4.0f;
        colorPickContaine_rect.corner_radius.top_right = 4.0f;
        colorPickContaine_rect.corner_radius.bottom_left = 4.0f;
        colorPickContaine_rect.corner_radius.bottom_right = 4.0f;
        auto &colorPickContainer_layout = colorPickContainer->getComponentMutable<core::components::Layout>();
        colorPickContainer_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        colorPickContainer_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        colorPickContainer_layout.padding.left = 16;
        colorPickContainer_layout.padding.right = 16;
        colorPickContainer_layout.padding.top = 16;
        colorPickContainer_layout.padding.bottom = 16;
        colorPickContainer->setParent(*option_panel);

        auto colorPicker = core::ecs::EntityRegistry::Create<core::ecs::entities::UIColorPicker>("Entity::UI::UIRect::UIColorPicker");
        colorPicker->setParent(*colorPickContainer);
        auto colorPickerHandle = colorPicker->getHandle();


        auto canvas_container = core::ecs::EntityRegistry::Create<core::ecs::entities::UI>("Entity::UI");
        auto &canvas_container_layout = canvas_container->getComponentMutable<core::components::Layout>();
        canvas_container_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        canvas_container_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        canvas_container_layout.child_alignment.vertical = core::components::Layout::ChildAlignment::Start;
        canvas_container->setParent(*texture_editor_container);

        auto canvas = core::ecs::EntityRegistry::Create<core::ecs::entities::UIDrawingCanvas>("Entity::UI::UIDrawingCanvas");
        auto &canvas_layout = canvas->getComponentMutable<core::components::Layout>();
        canvas_layout.z_index = 0;

        auto &canvasInfo = canvas->getComponentMutable<core::components::UIDrawingCanvas>();
        canvasInfo.canvas_size = { 1.0f, 1.0f };
        canvasInfo.zoom = 1.0f;
        canvasInfo.offset = { 0.0f, 0.0f };
        canvas->setParent(*canvas_container);

        auto widthHandle = widthNumberInput->getHandle();
        auto heightHandle = heightNumberInput->getHandle();
        canvas->getSignal<const core::types::Vector2i &>("New Dimensions").connect([widthHandle, heightHandle](const core::types::Vector2i &size) {
            if (!widthHandle.is_alive() || !heightHandle.is_alive()) {
                return;
            }

            core::ecs::entities::UINumberInput widthInput(core::ecs::EntityRegistry::GetEntityFromId(widthHandle));
            auto &widthInput_comp = widthInput.getComponentMutable<core::components::UINumberInput>();
            widthInput_comp.value = size.x;

            core::ecs::entities::UINumberInput heightInput(core::ecs::EntityRegistry::GetEntityFromId(heightHandle));
            auto &heightInput_comp = heightInput.getComponentMutable<core::components::UINumberInput>();
            heightInput_comp.value = size.y;
        });
        canvas->initPixelBuffer(128, 80);


        auto canvasHandle = canvas->getHandle();
        m_canvas_handle = canvasHandle;

        widthNumberInput->getSignal<int>("IntValueChanged").connect([canvasHandle](int val) {
            if (!canvasHandle.is_alive()) {
                return;
            }
            if (val < 1 || val > 10000) {
                spdlog::warn("Width is not inside 1 - 10000 bounds, clamped");
                val = common::math::Clamp(val, 1, 10000);
            }

            core::ecs::entities::UIDrawingCanvas canvas(core::ecs::EntityRegistry::GetEntityFromId(canvasHandle));
            auto &comp = canvas.getComponentMutable<core::components::UIDrawingCanvas>();

            canvas.resizeCanvas(val, comp.texture_size.y);
        });

        heightNumberInput->getSignal<int>("IntValueChanged").connect([canvasHandle](int val) {
            if (!canvasHandle.is_alive()) {
                return;
            }
            if (val < 1 || val > 10000) {
                spdlog::warn("Height is not inside 1 - 10000 bounds, clamped");
                val = common::math::Clamp(val, 1, 10000);
            }

            core::ecs::entities::UIDrawingCanvas canvas(core::ecs::EntityRegistry::GetEntityFromId(canvasHandle));
            auto &comp = canvas.getComponentMutable<core::components::UIDrawingCanvas>();

            canvas.resizeCanvas(comp.texture_size.x, val);
        });


        auto sizeSliderHandle = sizeSlider->getHandle();
        auto sizeNumberHandle = sizeNumberInput->getHandle();
        sizeSlider->getSignal<int>("IntValueChanged").connect([canvasHandle, sizeNumberHandle](int val) {
            if (!sizeNumberHandle.is_alive() || !canvasHandle.is_alive()) {
                return;
            }
            core::ecs::entities::UINumberInput nbInput(core::ecs::EntityRegistry::GetEntityFromId(sizeNumberHandle));
            auto &nbInput_comp = nbInput.getComponentMutable<core::components::UINumberInput>();
            nbInput_comp.value = val;

            core::ecs::entities::UIDrawingCanvas canvas(core::ecs::EntityRegistry::GetEntityFromId(canvasHandle));
            auto &canvas_comp = canvas.getComponentMutable<core::components::UIDrawingCanvas>();

            canvas_comp.brush_radius = val;
        });
        sizeNumberInput->getSignal<int>("IntValueChanged").connect([canvasHandle, sizeSliderHandle](int val) {
            if (!sizeSliderHandle.is_alive() || !canvasHandle.is_alive()) {
                return;
            }
            core::ecs::entities::UISlider slider(core::ecs::EntityRegistry::GetEntityFromId(sizeSliderHandle));
            slider.setValue(val, false);

            auto &comp = slider.getComponentMutable<core::components::UISlider>();
            int value = std::visit([](auto v) { return static_cast<int>(v); }, comp.max);
            if (val > value) {
                return;
            }

            core::ecs::entities::UIDrawingCanvas canvas(core::ecs::EntityRegistry::GetEntityFromId(canvasHandle));
            auto &canvas_comp = canvas.getComponentMutable<core::components::UIDrawingCanvas>();

            canvas_comp.brush_radius = val;
        });
        sizeSlider->getSignal<int>("IntValueChanged").emit(1);


        colorPicker->getSignal<core::types::Color>("ColorChanged").connect([canvasHandle](core::types::Color newColor) {
            if (!canvasHandle.is_alive()) {
                return;
            }
            core::ecs::entities::UIDrawingCanvas canvas(core::ecs::EntityRegistry::GetEntityFromId(canvasHandle));
            auto &canvas_comp = canvas.getComponentMutable<core::components::UIDrawingCanvas>();

            canvas_comp.brush_color = newColor;
        });
        auto &colorPicker_comp = colorPicker->getComponentMutable<core::components::UIColorPicker>();
        colorPicker->getSignal<core::types::Color>("ColorChanged").emit(colorPicker_comp.current_color);


        pencilBtn->getSignal<>("Pressed").connect([canvasHandle]() {
            if (!canvasHandle.is_alive()) {
                return;
            }
            core::ecs::entities::UIDrawingCanvas canvas(core::ecs::EntityRegistry::GetEntityFromId(canvasHandle));
            auto &canvas_comp = canvas.getComponentMutable<core::components::UIDrawingCanvas>();

            canvas_comp.pen = core::components::UIDrawingCanvas::DrawType::PENCIL;
        });

        eraserBtn->getSignal<>("Pressed").connect([canvasHandle]() {
            if (!canvasHandle.is_alive()) {
                return;
            }
            core::ecs::entities::UIDrawingCanvas canvas(core::ecs::EntityRegistry::GetEntityFromId(canvasHandle));
            auto &canvas_comp = canvas.getComponentMutable<core::components::UIDrawingCanvas>();

            canvas_comp.pen = core::components::UIDrawingCanvas::DrawType::ERASER;
        });

        fileExplorer->getSignal<std::string>("FileFocus").connect([this](std::string path) { open(path); });

        saveBtn->getSignal<>("Pressed").connect([this]() { save(); });
    }

    void TextureEditor::createTools() {}

    void TextureEditor::save()
    {
        if (!p_file_path || !m_canvas_handle.is_alive())
            return;

        core::ecs::entities::UIDrawingCanvas canvas(core::ecs::EntityRegistry::GetEntityFromId(m_canvas_handle));
        canvas.getComponentMutable<core::components::UIDrawingCanvas>().file_path = *p_file_path;
        canvas.saveCanvas();
    }

    void TextureEditor::load()
    {
        if (!p_file_path || !m_canvas_handle.is_alive())
            return;

        core::ecs::entities::UIDrawingCanvas canvas(core::ecs::EntityRegistry::GetEntityFromId(m_canvas_handle));
        canvas.importCanvas(*p_file_path);
    }
} // namespace atmo::editor

ATMO_REGISTER_EDITOR(atmo::editor::TextureEditor);
