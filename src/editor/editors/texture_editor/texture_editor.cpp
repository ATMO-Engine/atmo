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

        auto colorBtn = core::ecs::EntityRegistry::Create<core::ecs::entities::UIButton>("Entity::UI::UIRect::UIButton");
        ((core::ecs::entities::UILabel)colorBtn->getChildren()[0]).destroy();
        auto &colorBtn_rect = colorBtn->getComponentMutable<core::components::UIRect>();
        colorBtn_rect.border.color = core::types::Color::BLACK;
        colorBtn_rect.color = core::types::Color::RED;
        auto &colorBtn_layout = colorBtn->getComponentMutable<core::components::Layout>();
        colorBtn_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::FIXED;
        colorBtn_layout.width.size = core::components::Layout::SizingAxis::MinMax{ 26.0f, 26.0f };
        colorBtn_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::FIXED;
        colorBtn_layout.height.size = core::components::Layout::SizingAxis::MinMax{ 26.0f, 26.0f };
        colorBtn->setParent(*texture_editor_panel);


        auto textInput = core::ecs::EntityRegistry::Create<core::ecs::entities::UITextInput>("Entity::UI::UIInput::UITextInput");
        textInput->setParent(*texture_editor_panel);


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

        auto colorName = core::ecs::EntityRegistry::Create<core::ecs::entities::UILabel>("Entity::UI::UILabel");
        colorName->setText("R");
        colorName->setParent(*color_comp_panel);

        auto slider = core::ecs::EntityRegistry::Create<core::ecs::entities::UISlider>("Entity::UI::UIRect::UISlider");
        slider->setType(core::components::UISlider::SliderType::Float, 0.0f, 1.0f);
        slider->setParent(*color_comp_panel);

        auto numberInput = core::ecs::EntityRegistry::Create<core::ecs::entities::UINumberInput>("Entity::UI::UIInput::UINumberInput");
        auto &input_entity_comp = numberInput->getComponentMutable<core::components::UIInput>();
        input_entity_comp.input_type = atmo::core::components::UIInput::InputType::Float;
        numberInput->setParent(*color_comp_panel);

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
    }
} // namespace atmo::editor

ATMO_REGISTER_EDITOR(atmo::editor::TextureEditor);
