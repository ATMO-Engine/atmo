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

        auto texture_editor_panel = core::ecs::EntityRegistry::Create<core::ecs::entities::UIRect>("Entity::UI::UIRect");
        auto &texture_editor_panel_rect = texture_editor_panel->getComponentMutable<core::components::UIRect>();
        texture_editor_panel_rect.corner_radius.top_left = 5.0f;
        texture_editor_panel_rect.corner_radius.top_right = 5.0f;
        texture_editor_panel_rect.corner_radius.bottom_left = 5.0f;
        texture_editor_panel_rect.corner_radius.bottom_right = 5.0f;
        texture_editor_panel_rect.border.color = core::types::Color("#dbdbdb");
        texture_editor_panel_rect.color = core::types::Color("#dbdbdb");
        auto &texture_editor_panel_layout = texture_editor_panel->getComponentMutable<core::components::Layout>();
        texture_editor_panel_layout.direction = core::components::Layout::Direction::Vertical;
        texture_editor_panel_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::PERCENT;
        texture_editor_panel_layout.width.size = 0.2f;
        texture_editor_panel_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::PERCENT;
        texture_editor_panel_layout.height.size = 1.0f;
        texture_editor_panel_layout.padding.left = 16;
        texture_editor_panel_layout.padding.right = 16;
        texture_editor_panel_layout.padding.top = 16;
        texture_editor_panel_layout.padding.bottom = 16;
        texture_editor_panel_layout.child_gap = 16;
        texture_editor_panel->setParent(*texture_editor_container);

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

        auto sizeSliderHandle = sizeSlider->getHandle();
        auto sizeNumberHandle = sizeNumberInput->getHandle();
        sizeSlider->getSignal<int>("IntValueChanged").connect([this, sizeNumberHandle](int val) {
            if (!sizeNumberHandle.is_alive()) {
                return;
            }
            core::ecs::entities::UINumberInput nbInput(core::ecs::EntityRegistry::GetEntityFromId(sizeNumberHandle));
            auto &nbInput_comp = nbInput.getComponentMutable<core::components::UINumberInput>();
            nbInput_comp.value = val;

            this->m_brushSize = val;
        });
        sizeNumberInput->getSignal<int>("IntValueChanged").connect([this, sizeSliderHandle](int val) {
            if (!sizeSliderHandle.is_alive()) {
                return;
            }
            core::ecs::entities::UISlider slider(core::ecs::EntityRegistry::GetEntityFromId(sizeSliderHandle));
            slider.setValue(val, false);

            auto &comp = slider.getComponentMutable<core::components::UISlider>();
            int value = std::visit([](auto v) { return static_cast<int>(v); }, comp.max);
            if (val > value) {
                return;
            }

            this->m_brushSize = val;
        });
        sizeSlider->getSignal<int>("IntValueChanged").emit(m_brushSize);


        auto spacing_comp_panel = core::ecs::EntityRegistry::Create<core::ecs::entities::UIRect>("Entity::UI::UIRect");
        auto &spacing_comp_panel_rect = spacing_comp_panel->getComponentMutable<core::components::UIRect>();
        spacing_comp_panel_rect.corner_radius.top_left = 5.0f;
        spacing_comp_panel_rect.corner_radius.top_right = 5.0f;
        spacing_comp_panel_rect.corner_radius.bottom_left = 5.0f;
        spacing_comp_panel_rect.corner_radius.bottom_right = 5.0f;
        spacing_comp_panel_rect.border.color = core::types::Color("#7d7d7d");
        spacing_comp_panel_rect.color = core::types::Color("#7d7d7d");
        auto &spacing_comp_panel_layout = spacing_comp_panel->getComponentMutable<core::components::Layout>();
        spacing_comp_panel_layout.direction = core::components::Layout::Direction::Horizontal;
        spacing_comp_panel_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        spacing_comp_panel_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::FIT;
        spacing_comp_panel_layout.padding.left = 16;
        spacing_comp_panel_layout.padding.right = 16;
        spacing_comp_panel_layout.padding.top = 16;
        spacing_comp_panel_layout.padding.bottom = 16;
        spacing_comp_panel_layout.child_gap = 0;
        spacing_comp_panel->setParent(*texture_editor_panel);

        auto spacingLabel = core::ecs::EntityRegistry::Create<core::ecs::entities::UILabel>("Entity::UI::UILabel");
        spacingLabel->setText("Spacing");
        spacingLabel->setFontSize(16);
        spacingLabel->setParent(*spacing_comp_panel);

        auto spacingSlider = core::ecs::EntityRegistry::Create<core::ecs::entities::UISlider>("Entity::UI::UIRect::UISlider");
        spacingSlider->setType(core::components::UISlider::SliderType::Float, 0.1f, 1.0f);
        spacingSlider->setParent(*spacing_comp_panel);

        auto spacingNumberInput = core::ecs::EntityRegistry::Create<core::ecs::entities::UINumberInput>("Entity::UI::UIInput::UINumberInput");
        auto &spacing_input_entity_comp = spacingNumberInput->getComponentMutable<core::components::UIInput>();
        spacing_input_entity_comp.input_type = atmo::core::components::UIInput::InputType::Float;
        spacingNumberInput->setParent(*spacing_comp_panel);


        auto spacingSliderHandle = spacingSlider->getHandle();
        auto spacingNumberHandle = spacingNumberInput->getHandle();
        spacingSlider->getSignal<float>("FloatValueChanged").connect([this, spacingNumberHandle](float val) {
            if (!spacingNumberHandle.is_alive()) {
                return;
            }
            core::ecs::entities::UINumberInput nbInput(core::ecs::EntityRegistry::GetEntityFromId(spacingNumberHandle));
            auto button = core::ecs::entities::UIButton(nbInput.getChildren()[0]);
            if (!button.getHandle().is_alive()) {
                return;
            }
            auto label = core::ecs::entities::UILabel(button.getChildren()[0]);
            if (!label.getHandle().is_alive()) {
                return;
            }
            auto &nbInput_comp = nbInput.getComponentMutable<core::components::UIInput>();
            nbInput_comp.input_data = std::format("{}", val);
            nbInput.validateInput(); // this trigger FloatValueChanged from uiInput !!!! can cause infinite loop easily
            label.setText(nbInput_comp.input_data);

            this->m_brushSpacing = val;
        });
        spacingSlider->getSignal<float>("FloatValueChanged").emit(m_brushSpacing);
        spacingNumberInput->getSignal<float>("FloatValueChanged").connect([this, spacingSliderHandle](float val) {
            if (!spacingSliderHandle.is_alive()) {
                return;
            }
            core::ecs::entities::UISlider slider(core::ecs::EntityRegistry::GetEntityFromId(spacingSliderHandle));
            slider.setValue(val, false);

            auto &comp = slider.getComponentMutable<core::components::UISlider>();
            float value = std::visit([](auto v) { return static_cast<float>(v); }, comp.max);
            if (val > value) {
                return;
            }

            this->m_brushSpacing = val;
        });


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


        auto exportBtn = core::ecs::EntityRegistry::Create<core::ecs::entities::UIButton>("Entity::UI::UIRect::UIButton");
        ((core::ecs::entities::UILabel)exportBtn->getChildren()[0]).setText("Export");
        auto &exportBtn_rect = exportBtn->getComponentMutable<core::components::UIRect>();
        exportBtn_rect.border.color = core::types::Color::BLACK;
        exportBtn_rect.color = core::types::Color::WHITE;
        auto &exportBtn_layout = exportBtn->getComponentMutable<core::components::Layout>();
        exportBtn_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::PERCENT;
        exportBtn_layout.width.size = 0.30f;
        exportBtn_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::PERCENT;
        exportBtn_layout.height.size = 0.05f;
        exportBtn->setParent(*texture_editor_panel);

        auto importBtn = core::ecs::EntityRegistry::Create<core::ecs::entities::UIButton>("Entity::UI::UIRect::UIButton");
        ((core::ecs::entities::UILabel)importBtn->getChildren()[0]).setText("Import");
        auto &importBtn_rect = importBtn->getComponentMutable<core::components::UIRect>();
        importBtn_rect.border.color = core::types::Color::BLACK;
        importBtn_rect.color = core::types::Color::WHITE;
        auto &importBtn_layout = importBtn->getComponentMutable<core::components::Layout>();
        importBtn_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::PERCENT;
        importBtn_layout.width.size = 0.30f;
        importBtn_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::PERCENT;
        importBtn_layout.height.size = 0.05f;
        importBtn->setParent(*texture_editor_panel);

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

        auto colorPicker = core::ecs::EntityRegistry::Create<core::ecs::entities::UIColorPicker>("Entity::UI::UIRect::UIColorPicker");
        colorPicker->setParent(*texture_editor_panel);
        auto colorPickerHandle = colorPicker->getHandle();
        colorPicker->getSignal<core::types::Color>("ColorChanged").connect([this](core::types::Color newColor) { this->m_brushColor = newColor; });
        auto &colorPicker_comp = colorPicker->getComponentMutable<core::components::UIColorPicker>();
        colorPicker->getSignal<core::types::Color>("ColorChanged").emit(colorPicker_comp.current_color);


        auto canvas_container = core::ecs::EntityRegistry::Create<core::ecs::entities::UI>("Entity::UI");
        auto &canvas_container_layout = canvas_container->getComponentMutable<core::components::Layout>();
        canvas_container_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        canvas_container_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        canvas_container_layout.child_alignment.vertical = core::components::Layout::ChildAlignment::Start;
        canvas_container->setParent(*texture_editor_container);

        auto canvas = core::ecs::EntityRegistry::Create<core::ecs::entities::UIDrawingCanvas>("Entity::UI::UIDrawingCanvas");
        canvas->getSignal<core::ecs::entities::UIDrawingCanvas &>("FetchBrush").connect([this](core::ecs::entities::UIDrawingCanvas &canvas) {
            auto &comp = canvas.getComponentMutable<core::components::UIDrawingCanvas>();

            comp.brushColor = this->m_brushColor;
            comp.brushRadius = this->m_brushSize;
            comp.brushSpacing = this->m_brushSpacing;
        });

        auto &canvas_layout = canvas->getComponentMutable<core::components::Layout>();
        canvas_layout.z_index = 0;

        auto &canvasInfo = canvas->getComponentMutable<core::components::UIDrawingCanvas>();
        canvasInfo.canvasSize = { 1.0f, 1.0f };
        canvasInfo.zoom = 1.0f;
        canvasInfo.offset = { 0.0f, 0.0f };
        canvas->setParent(*canvas_container);

        auto widthHandle = widthNumberInput->getHandle();
        auto heightHandle = heightNumberInput->getHandle();
        canvas->getSignal<const core::types::Vector2 &>("New Dimensions").connect([widthHandle, heightHandle](const core::types::Vector2 &size) {
            if (!widthHandle.is_alive() || !heightHandle.is_alive()) {
                return;
            }
            {
                core::ecs::entities::UINumberInput widthInput(core::ecs::EntityRegistry::GetEntityFromId(widthHandle));
                auto button = core::ecs::entities::UIButton(widthInput.getChildren()[0]);
                if (!button.getHandle().is_alive()) {
                    return;
                }
                auto label = core::ecs::entities::UILabel(button.getChildren()[0]);
                if (!label.getHandle().is_alive()) {
                    return;
                }
                auto &nbInput_comp = widthInput.getComponentMutable<core::components::UIInput>();
                nbInput_comp.input_data = std::format("{}", size.x);
                widthInput.validateInput(); // this trigger FloatValueChanged from uiInput !!!! can cause infinite loop easily
                label.setText(nbInput_comp.input_data);
            }
            {
                core::ecs::entities::UINumberInput heightInput(core::ecs::EntityRegistry::GetEntityFromId(heightHandle));
                auto button = core::ecs::entities::UIButton(heightInput.getChildren()[0]);
                if (!button.getHandle().is_alive()) {
                    return;
                }
                auto label = core::ecs::entities::UILabel(button.getChildren()[0]);
                if (!label.getHandle().is_alive()) {
                    return;
                }
                auto &nbInput_comp = heightInput.getComponentMutable<core::components::UIInput>();
                nbInput_comp.input_data = std::format("{}", size.y);
                heightInput.validateInput(); // this trigger FloatValueChanged from uiInput !!!! can cause infinite loop easily
                label.setText(nbInput_comp.input_data);
            }
        });
        canvas->initPixelBuffer(128, 80);


        auto canvasHandle = canvas->getHandle();

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

            canvas.resizeCanvas(val, comp.textureSize.y);
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

            canvas.resizeCanvas(comp.textureSize.x, val);
        });

        exportBtn->getSignal<>("Pressed").connect([canvasHandle]() {
            if (!canvasHandle.is_alive()) {
                return;
            }

            core::ecs::entities::UIDrawingCanvas canvas(core::ecs::EntityRegistry::GetEntityFromId(canvasHandle));
            canvas.exportCanvas("assets/save.png");
        });

        importBtn->getSignal<>("Pressed").connect([canvasHandle]() {
            if (!canvasHandle.is_alive()) {
                return;
            }

            core::ecs::entities::UIDrawingCanvas canvas(core::ecs::EntityRegistry::GetEntityFromId(canvasHandle));
            canvas.importCanvas("assets/save.png");
        });
    }
} // namespace atmo::editor

ATMO_REGISTER_EDITOR(atmo::editor::TextureEditor);
