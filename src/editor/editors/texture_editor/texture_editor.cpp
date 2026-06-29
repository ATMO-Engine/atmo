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
        texture_editor_container_layout.child_alignment.vertical = core::components::Layout::ChildAlignment::End;
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

        auto color_comp_panel = core::ecs::EntityRegistry::Create<core::ecs::entities::UIRect>("Entity::UI::UIRect");
        auto &color_comp_panel_rect = color_comp_panel->getComponentMutable<core::components::UIRect>();
        color_comp_panel_rect.corner_radius.top_left = 5.0f;
        color_comp_panel_rect.corner_radius.top_right = 5.0f;
        color_comp_panel_rect.corner_radius.bottom_left = 5.0f;
        color_comp_panel_rect.corner_radius.bottom_right = 5.0f;
        color_comp_panel_rect.border.color = core::types::Color("#7d7d7d");
        color_comp_panel_rect.color = core::types::Color("#7d7d7d");
        auto &color_comp_panel_layout = color_comp_panel->getComponentMutable<core::components::Layout>();
        color_comp_panel_layout.direction = core::components::Layout::Direction::Horizontal;
        color_comp_panel_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        color_comp_panel_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::FIT;
        color_comp_panel_layout.padding.left = 16;
        color_comp_panel_layout.padding.right = 16;
        color_comp_panel_layout.padding.top = 16;
        color_comp_panel_layout.padding.bottom = 16;
        color_comp_panel_layout.child_gap = 0;
        color_comp_panel->setParent(*texture_editor_panel);

        auto sizeLabel = core::ecs::EntityRegistry::Create<core::ecs::entities::UILabel>("Entity::UI::UILabel");
        sizeLabel->setText("Size");
        sizeLabel->setFontSize(16);
        sizeLabel->setParent(*color_comp_panel);

        auto sizeSlider = core::ecs::EntityRegistry::Create<core::ecs::entities::UISlider>("Entity::UI::UIRect::UISlider");
        sizeSlider->setType(core::components::UISlider::SliderType::Int, 1, 10);
        sizeSlider->setParent(*color_comp_panel);

        auto numberInput = core::ecs::EntityRegistry::Create<core::ecs::entities::UINumberInput>("Entity::UI::UIInput::UINumberInput");
        auto &input_entity_comp = numberInput->getComponentMutable<core::components::UIInput>();
        input_entity_comp.input_type = atmo::core::components::UIInput::InputType::Int;
        numberInput->setParent(*color_comp_panel);


        auto sliderHandle = sizeSlider->getHandle();
        auto numberHandle = numberInput->getHandle();
        sizeSlider->getSignal<int>("IntValueChanged").connect([this, numberHandle](int val) {
            if (!numberHandle.is_alive()) {
                return;
            }
            core::ecs::entities::UINumberInput nbInput(core::ecs::EntityRegistry::GetEntityFromId(numberHandle));
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
            nbInput.validateInput();
            label.setText(nbInput_comp.input_data);

            this->brushSize = val;
        });
        sizeSlider->getSignal<int>("IntValueChanged").emit(1);
        numberInput->getSignal<int>("IntValueChanged").connect([this, sliderHandle](int val) {
            if (!sliderHandle.is_alive()) {
                return;
            }
            core::ecs::entities::UISlider slider(core::ecs::EntityRegistry::GetEntityFromId(sliderHandle));
            slider.setValue(val, false);

            auto &comp = slider.getComponentMutable<core::components::UISlider>();
            int value = std::visit([](auto v) { return static_cast<int>(v); }, comp.max);
            if (val > value) {
                return;
            }

            this->brushSize = val;
        });


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
        colorPicker->getSignal<core::types::Color>("ColorChanged").connect([this](core::types::Color newColor) { this->brushColor = newColor; });


        auto canvas = core::ecs::EntityRegistry::Create<core::ecs::entities::UIDrawingCanvas>("Entity::UI::UIDrawingCanvas");
        canvas->getSignal<core::ecs::entities::UIDrawingCanvas &>("FetchBrush").connect([this](core::ecs::entities::UIDrawingCanvas &canvas) {
            auto &comp = canvas.getComponentMutable<core::components::UIDrawingCanvas>();

            comp.brushColor = this->brushColor;
            comp.brushRadius = this->brushSize;
        });

        auto &canvasInfo = canvas->getComponentMutable<core::components::UIDrawingCanvas>();
        canvasInfo.textureSize = { 128, 80 };
        canvasInfo.canvasSize = { 0.40f, 0.30f };
        canvasInfo.zoom = 1.0f;
        canvasInfo.offset = { 0.0f, 0.0f };

        auto windowEntity = container.getWindow();
        auto &windowComp = windowEntity->getComponentMutable<core::components::Window>();
        if (!windowComp.renderer_data.renderer) {
            return;
        }

        { /* Create the texture where the user draw */
            canvasInfo.drawing_texture = SDL_CreateTexture(
                windowComp.renderer_data.renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, canvasInfo.textureSize.x, canvasInfo.textureSize.y);
            SDL_SetTextureScaleMode(canvasInfo.drawing_texture, SDL_SCALEMODE_NEAREST);

            SDL_SetRenderTarget(windowComp.renderer_data.renderer, canvasInfo.drawing_texture);
            SDL_SetRenderDrawColor(windowComp.renderer_data.renderer, 0, 0, 0, 0);
            SDL_RenderClear(windowComp.renderer_data.renderer);
            SDL_SetRenderTarget(windowComp.renderer_data.renderer, nullptr);
        }

        { /* Create a checkboard texture to render behind the drawing texture */
            canvasInfo.checkerboard_texture = SDL_CreateTexture(
                windowComp.renderer_data.renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, canvasInfo.textureSize.x, canvasInfo.textureSize.y);
            SDL_SetTextureScaleMode(canvasInfo.checkerboard_texture, SDL_SCALEMODE_NEAREST);

            SDL_SetRenderTarget(windowComp.renderer_data.renderer, canvasInfo.checkerboard_texture);
            for (int y = 0; y < canvasInfo.textureSize.y; ++y) {
                for (int x = 0; x < canvasInfo.textureSize.x; ++x) {
                    bool dark = ((x + y) % 2) == 0;

                    if (dark) {
                        SDL_SetRenderDrawColor(windowComp.renderer_data.renderer, 180, 180, 180, 255);
                    } else {
                        SDL_SetRenderDrawColor(windowComp.renderer_data.renderer, 230, 230, 230, 255);
                    }

                    SDL_RenderPoint(windowComp.renderer_data.renderer, (float)x, (float)y);
                }
            }
            SDL_SetRenderTarget(windowComp.renderer_data.renderer, nullptr);
        }
        canvas->setParent(*texture_editor_container);

        auto canvasHandle = canvas->getHandle();
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
