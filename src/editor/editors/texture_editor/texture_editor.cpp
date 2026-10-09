#include "texture_editor.hpp"
#include <algorithm>
#include <filesystem>
#include <format>
#include <variant>
#include "core/ecs/entities/ui/ui.hpp"
#include "core/ecs/entities/ui/ui_button/ui_button.hpp"
#include "core/ecs/entities/ui/ui_checkbox/ui_checkbox.hpp"
#include "core/ecs/entities/ui/ui_image/ui_image.hpp"
#include "core/ecs/entities/ui/ui_input/ui_number_input/ui_number_input.hpp"
#include "core/ecs/entities/ui/ui_input/ui_text_input/ui_text_input.hpp"
#include "core/ecs/entities/ui/ui_label/ui_label.hpp"
#include "core/ecs/entities/ui/ui_layout.hpp"
#include "core/ecs/entities/ui/ui_rect/ui_rect.hpp"
#include "core/ecs/entities/ui/ui_slider/ui_slider.hpp"
#include "core/event/event_registry.hpp"
#include "core/event/events/progress_tick_event/progress_tick_event.hpp"
#include "core/input/input_manager.hpp"
#include "core/types.hpp"
#include "editor/editor_entities/ui_color_picker/ui_color_picker.hpp"
#include "editor/editor_entities/ui_drawing_canvas/ui_drawing_canvas.hpp"
#include "editor/editor_entities/ui_file_explorer/ui_file_explorer.hpp"
#include "editor/editor_entities/ui_grid/ui_grid.hpp"
#include "editor/editor_entities/ui_popup/ui_popup.hpp"
#include "editor/editor_registry.hpp"
#include "project/file_system.hpp"
#include "project/project_manager.hpp"

namespace atmo::editor
{
    void TextureEditor::init(atmo::core::ecs::entities::UI &container)
    {
        m_root_handle = container.getHandle();

        flecs::entity root = container.getHandle().world().lookup("_Root");
        SDL_Renderer *renderer = nullptr;
        if (root.is_valid() && root.has<core::components::Window>()) {
            auto window = root.get_ref<core::components::Window>();
            if (window)
                renderer = window->renderer_data.renderer;
        }
        m_scene_ctx = std::make_unique<EditorSceneContext>();
        m_scene_ctx->init(renderer);

        if (m_scene_ctx && m_scene_ctx->isReady()) {
            auto viewport_image = core::ecs::EntityRegistry::Create<core::ecs::entities::UIImage>("Entity::UI::UIImage");
            auto &viewport_img_comp = viewport_image->getComponentMutable<core::components::UIImage>();
            auto &viewport_image_layout = viewport_image->getComponentMutable<core::components::Layout>();

            viewport_img_comp.raw_texture = m_scene_ctx->getViewportTexture();
            viewport_image_layout.floating.enabled = true;
            viewport_image_layout.z_index = -1;
            viewport_image_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
            viewport_image_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
            viewport_image->setParent(container);
            m_viewport_image = viewport_image->getHandle();
        } else {
            spdlog::error("Couldn't create scene viewport");
        }

        core::event::EventRegistry::SetCallBack<core::event::events::ProgressTickEvent>(
            [this, ctx = m_scene_ctx.get(), handle = root, vp_img = m_viewport_image](core::event::events::ProgressTickEvent *evt) {
                SDL_Renderer *renderer = nullptr;
                if (handle.is_valid() && handle.has<core::components::Window>()) {
                    auto window = handle.get_ref<core::components::Window>();
                    if (window) {
                        renderer = window->renderer_data.renderer;

                        if (vp_img.is_valid() && vp_img.has<core::components::UIImage>()) {
                            auto img = vp_img.get_ref<core::components::UIImage>();
                            const int w = static_cast<int>(img->rendered_size[0]);
                            const int h = static_cast<int>(img->rendered_size[1]);
                            if (w > 0 && h > 0) {
                                ctx->resize(w, h);
                                img->raw_texture = ctx->getViewportTexture();
                            }
                        }

                        ctx->tick(evt->delta_time, renderer);
                    }
                }

                auto [scroll, scroll_dt] = core::InputManager::GetScrollDelta("ui_scroll");
                if (scroll.x != 0.0f || scroll.y != 0.0f) {
#if defined(__APPLE__)
                    const bool ctrl_held = SDL_GetModState() & SDL_KMOD_GUI;
#else
                    const bool ctrl_held = SDL_GetModState() & SDL_KMOD_CTRL;
#endif
                    if (ctrl_held) {
                        const float factor = std::pow(1.12f, scroll.y);
                        ctx->zoom(factor, { ctx->getWidth() * 0.5f, ctx->getHeight() * 0.5f });
                    } else {
                        ctx->pan({ -scroll.x * 5.0f, scroll.y * 5.0f });
                    }
                }

                float pinch = core::InputManager::GetPinchScale("ui_pinch");
                if (pinch != 0.0f)
                    ctx->zoom(pinch, { ctx->getWidth() * 0.5f, ctx->getHeight() * 0.5f });

                for (auto &fn : m_inspector_update_fns) fn();

                tickAnimation(evt->delta_time);
            });

        auto texture_editor_container = core::ecs::EntityRegistry::Create<core::ecs::entities::UI>("Entity::UI");
        auto &texture_editor_container_layout = texture_editor_container->getComponentMutable<core::components::Layout>();

        texture_editor_container_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        texture_editor_container_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        texture_editor_container_layout.z_index = 0;
        texture_editor_container_layout.padding = { 16, 16, 8, 16 };
        texture_editor_container_layout.child_gap = 8;
        texture_editor_container_layout.child_alignment.vertical = core::components::Layout::ChildAlignment::Start;
        texture_editor_container->setParent(container);

        auto option_panel = core::ecs::EntityRegistry::Create<core::ecs::entities::UIRect>("Entity::UI::UIRect");
        auto &option_panel_rect = option_panel->getComponentMutable<core::components::UIRect>();
        auto &option_panel_layout = option_panel->getComponentMutable<core::components::Layout>();

        option_panel_rect.color = core::types::Color::WHITE;
        option_panel_layout.direction = core::components::Layout::Direction::Vertical;
        option_panel_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::PERCENT;
        option_panel_layout.width.size = 0.2f;
        option_panel_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        option_panel_layout.child_gap = 16;
        option_panel->setParent(*texture_editor_container);

        auto texture_editor_panel = core::ecs::EntityRegistry::Create<core::ecs::entities::UIRect>("Entity::UI::UIRect");
        auto &texture_editor_panel_rect = texture_editor_panel->getComponentMutable<core::components::UIRect>();
        auto &texture_editor_panel_layout = texture_editor_panel->getComponentMutable<core::components::Layout>();

        texture_editor_panel_rect.corner_radius.top_left = 4.0f;
        texture_editor_panel_rect.corner_radius.top_right = 4.0f;
        texture_editor_panel_rect.corner_radius.bottom_left = 4.0f;
        texture_editor_panel_rect.corner_radius.bottom_right = 4.0f;
        texture_editor_panel_rect.color = core::types::Color::TRANSPARENT_COL;
        texture_editor_panel_layout.direction = core::components::Layout::Direction::Vertical;
        texture_editor_panel_layout.child_alignment.horizontal = core::components::Layout::ChildAlignment::Start;
        texture_editor_panel_layout.child_alignment.vertical = core::components::Layout::ChildAlignment::Center;
        texture_editor_panel_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        texture_editor_panel_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::PERCENT;
        texture_editor_panel_layout.height.size = 0.45f;
        texture_editor_panel_layout.padding.left = 16;
        texture_editor_panel_layout.padding.right = 16;
        texture_editor_panel_layout.padding.top = 16;
        texture_editor_panel_layout.padding.bottom = 16;
        texture_editor_panel_layout.child_gap = 16;
        texture_editor_panel->setParent(*option_panel);

        auto size_comp_panel = core::ecs::EntityRegistry::Create<core::ecs::entities::UIRect>("Entity::UI::UIRect");
        auto &size_comp_panel_rect = size_comp_panel->getComponentMutable<core::components::UIRect>();
        auto &size_comp_panel_layout = size_comp_panel->getComponentMutable<core::components::Layout>();

        size_comp_panel_rect.corner_radius.top_left = 5.0f;
        size_comp_panel_rect.corner_radius.top_right = 5.0f;
        size_comp_panel_rect.corner_radius.bottom_left = 5.0f;
        size_comp_panel_rect.corner_radius.bottom_right = 5.0f;
        size_comp_panel_rect.color = core::types::Color::TRANSPARENT_COL;
        size_comp_panel_layout.direction = core::components::Layout::Direction::Horizontal;
        size_comp_panel_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        size_comp_panel_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::FIT;
        size_comp_panel_layout.padding.left = 16;
        size_comp_panel_layout.padding.right = 16;
        size_comp_panel_layout.padding.top = 16;
        size_comp_panel_layout.padding.bottom = 16;
        size_comp_panel_layout.child_gap = 8;
        size_comp_panel->setParent(*texture_editor_panel);

        auto sizeLabel = core::ecs::EntityRegistry::Create<core::ecs::entities::UILabel>("Entity::UI::UILabel");
        sizeLabel->setText("Size");
        sizeLabel->setFontSize(12);
        sizeLabel->getComponentMutable<core::components::UI>().modulate = core::types::Color::BLACK;
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
        auto &width_comp_panel_layout = width_comp_panel->getComponentMutable<core::components::Layout>();

        width_comp_panel_rect.corner_radius.top_left = 5.0f;
        width_comp_panel_rect.corner_radius.top_right = 5.0f;
        width_comp_panel_rect.corner_radius.bottom_left = 5.0f;
        width_comp_panel_rect.corner_radius.bottom_right = 5.0f;
        width_comp_panel_rect.color = core::types::Color::TRANSPARENT_COL;
        width_comp_panel_layout.direction = core::components::Layout::Direction::Horizontal;
        width_comp_panel_layout.child_alignment.horizontal = core::components::Layout::ChildAlignment::Start;
        width_comp_panel_layout.child_alignment.vertical = core::components::Layout::ChildAlignment::Center;
        width_comp_panel_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        width_comp_panel_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::FIT;
        width_comp_panel_layout.padding.left = 16;
        width_comp_panel_layout.padding.right = 16;
        width_comp_panel_layout.padding.top = 16;
        width_comp_panel_layout.padding.bottom = 16;
        width_comp_panel_layout.child_gap = 8;
        width_comp_panel->setParent(*texture_editor_panel);

        auto widthLabel = core::ecs::EntityRegistry::Create<core::ecs::entities::UILabel>("Entity::UI::UILabel");
        widthLabel->setText("Width");
        widthLabel->setFontSize(12);
        widthLabel->getComponentMutable<core::components::UI>().modulate = core::types::Color::BLACK;
        widthLabel->setParent(*width_comp_panel);

        auto widthNumberInput = core::ecs::EntityRegistry::Create<core::ecs::entities::UINumberInput>("Entity::UI::UIInput::UINumberInput");
        auto &width_input_entity_comp = widthNumberInput->getComponentMutable<core::components::UIInput>();
        width_input_entity_comp.input_type = atmo::core::components::UIInput::InputType::Int;
        widthNumberInput->setParent(*width_comp_panel);

        auto height_comp_panel = core::ecs::EntityRegistry::Create<core::ecs::entities::UIRect>("Entity::UI::UIRect");
        auto &height_comp_panel_rect = height_comp_panel->getComponentMutable<core::components::UIRect>();
        auto &height_comp_panel_layout = height_comp_panel->getComponentMutable<core::components::Layout>();

        height_comp_panel_rect.corner_radius.top_left = 5.0f;
        height_comp_panel_rect.corner_radius.top_right = 5.0f;
        height_comp_panel_rect.corner_radius.bottom_left = 5.0f;
        height_comp_panel_rect.corner_radius.bottom_right = 5.0f;
        height_comp_panel_rect.color = core::types::Color::TRANSPARENT_COL;
        height_comp_panel_layout.direction = core::components::Layout::Direction::Horizontal;
        height_comp_panel_layout.child_alignment.horizontal = core::components::Layout::ChildAlignment::Start;
        height_comp_panel_layout.child_alignment.vertical = core::components::Layout::ChildAlignment::Center;
        height_comp_panel_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        height_comp_panel_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::FIT;
        height_comp_panel_layout.padding.left = 16;
        height_comp_panel_layout.padding.right = 16;
        height_comp_panel_layout.padding.top = 16;
        height_comp_panel_layout.padding.bottom = 16;
        height_comp_panel_layout.child_gap = 8;
        height_comp_panel->setParent(*texture_editor_panel);

        auto heightLabel = core::ecs::EntityRegistry::Create<core::ecs::entities::UILabel>("Entity::UI::UILabel");
        heightLabel->setText("Height");
        heightLabel->setFontSize(12);
        heightLabel->getComponentMutable<core::components::UI>().modulate = core::types::Color::BLACK;
        heightLabel->setParent(*height_comp_panel);

        auto heightNumberInput = core::ecs::EntityRegistry::Create<core::ecs::entities::UINumberInput>("Entity::UI::UIInput::UINumberInput");
        auto &height_input_entity_comp = heightNumberInput->getComponentMutable<core::components::UIInput>();
        height_input_entity_comp.input_type = atmo::core::components::UIInput::InputType::Int;
        heightNumberInput->setParent(*height_comp_panel);


        auto saveBtn = core::ecs::EntityRegistry::Create<core::ecs::entities::UIButton>("Entity::UI::UIRect::UIButton");
        auto saveBtn_label = core::ecs::EntityRegistry::Create<core::ecs::entities::UILabel>("Entity::UI::UILabel");
        auto &saveBtn_rect = saveBtn->getComponentMutable<core::components::UIRect>();
        auto &saveBtn_layout = saveBtn->getComponentMutable<core::components::Layout>();

        saveBtn_label->setText("Save");
        saveBtn_label->getComponentMutable<core::components::UI>().modulate = core::types::Color::BLACK;
        saveBtn_label->setFontSize(12);
        saveBtn_rect.color = core::types::Color::TRANSPARENT_COL;
        saveBtn_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::PERCENT;
        saveBtn_layout.width.size = 0.35f;
        saveBtn_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::PERCENT;
        saveBtn_layout.height.size = 0.10f;
        saveBtn_layout.child_alignment.horizontal = core::components::Layout::ChildAlignment::Start;
        saveBtn_layout.padding.left = 12;
        saveBtn_label->setParent(*saveBtn);
        saveBtn->setParent(*texture_editor_panel);

        auto exportBtn = core::ecs::EntityRegistry::Create<core::ecs::entities::UIButton>("Entity::UI::UIRect::UIButton");
        auto exportBtn_label = core::ecs::EntityRegistry::Create<core::ecs::entities::UILabel>("Entity::UI::UILabel");
        auto &exportBtn_rect = exportBtn->getComponentMutable<core::components::UIRect>();
        auto &exportBtn_layout = exportBtn->getComponentMutable<core::components::Layout>();

        exportBtn_label->setText("Export");
        exportBtn_label->getComponentMutable<core::components::UI>().modulate = core::types::Color::BLACK;
        exportBtn_label->setFontSize(12);
        exportBtn_rect.color = core::types::Color::TRANSPARENT_COL;
        exportBtn_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::PERCENT;
        exportBtn_layout.width.size = 0.35f;
        exportBtn_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::PERCENT;
        exportBtn_layout.height.size = 0.10f;
        exportBtn_layout.child_alignment.horizontal = core::components::Layout::ChildAlignment::Start;
        exportBtn_layout.padding.left = 12;
        exportBtn_label->setParent(*exportBtn);
        exportBtn->setParent(*texture_editor_panel);

        auto previewBtn = core::ecs::EntityRegistry::Create<core::ecs::entities::UIButton>("Entity::UI::UIRect::UIButton");
        auto previewBtn_label = core::ecs::EntityRegistry::Create<core::ecs::entities::UILabel>("Entity::UI::UILabel");
        auto &previewBtn_rect = previewBtn->getComponentMutable<core::components::UIRect>();
        auto &previewBtn_layout = previewBtn->getComponentMutable<core::components::Layout>();

        previewBtn_label->setText("Preview");
        previewBtn_label->getComponentMutable<core::components::UI>().modulate = core::types::Color::BLACK;
        previewBtn_label->setFontSize(12);
        previewBtn_rect.color = core::types::Color::TRANSPARENT_COL;
        previewBtn_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::PERCENT;
        previewBtn_layout.width.size = 0.35f;
        previewBtn_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::PERCENT;
        previewBtn_layout.height.size = 0.10f;
        previewBtn_layout.child_alignment.horizontal = core::components::Layout::ChildAlignment::Start;
        previewBtn_layout.padding.left = 12;
        previewBtn_label->setParent(*previewBtn);
        previewBtn->setParent(*texture_editor_panel);

        auto pencilContainer = core::ecs::EntityRegistry::Create<core::ecs::entities::UIRect>("Entity::UI::UIRect");
        auto &btnContainer_rect = pencilContainer->getComponentMutable<core::components::UIRect>();
        auto &pencilContainer_layout = pencilContainer->getComponentMutable<core::components::Layout>();

        btnContainer_rect.color.a = 0;
        pencilContainer_layout.direction = core::components::Layout::Direction::Horizontal;
        pencilContainer_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        pencilContainer_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::PERCENT;
        pencilContainer_layout.height.size = 0.05f;
        pencilContainer_layout.child_gap = 16;
        pencilContainer_layout.child_alignment.horizontal = core::components::Layout::ChildAlignment::Start;
        pencilContainer->setParent(*texture_editor_panel);

        auto pencilBtn = core::ecs::EntityRegistry::Create<core::ecs::entities::UIButton>("Entity::UI::UIRect::UIButton");
        auto pencilBtn_label = core::ecs::EntityRegistry::Create<core::ecs::entities::UILabel>("Entity::UI::UILabel");
        auto &pencilBtn_rect = pencilBtn->getComponentMutable<core::components::UIRect>();
        auto &pencilBtn_layout = pencilBtn->getComponentMutable<core::components::Layout>();
        auto &pencilBtn_btn_comp = pencilBtn->getComponentMutable<core::components::UIButton>();

        pencilBtn_btn_comp.toggle = true;
        pencilBtn_btn_comp.group = 5;
        pencilBtn_label->setText("Draw");
        pencilBtn_label->getComponentMutable<core::components::UI>().modulate = core::types::Color::BLACK;
        pencilBtn_label->setFontSize(12);
        pencilBtn_rect.color = core::types::Color::TRANSPARENT_COL;
        pencilBtn_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::PERCENT;
        pencilBtn_layout.width.size = 0.35f;
        pencilBtn_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        pencilBtn_layout.child_alignment.horizontal = core::components::Layout::ChildAlignment::Start;
        pencilBtn_layout.padding.left = 12;
        pencilBtn_label->setParent(*pencilBtn);
        pencilBtn->setParent(*pencilContainer);

        auto eraserBtn = core::ecs::EntityRegistry::Create<core::ecs::entities::UIButton>("Entity::UI::UIRect::UIButton");
        auto eraserBtn_label = core::ecs::EntityRegistry::Create<core::ecs::entities::UILabel>("Entity::UI::UILabel");
        auto &eraserBtn_rect = eraserBtn->getComponentMutable<core::components::UIRect>();
        auto &eraserBtn_layout = eraserBtn->getComponentMutable<core::components::Layout>();
        auto &eraserBtn_btn_comp = eraserBtn->getComponentMutable<core::components::UIButton>();

        eraserBtn_btn_comp.toggle = true;
        eraserBtn_btn_comp.group = 5;
        eraserBtn_label->setText("Erase");
        eraserBtn_label->getComponentMutable<core::components::UI>().modulate = core::types::Color::BLACK;
        eraserBtn_label->setFontSize(12);
        eraserBtn_rect.color = core::types::Color::TRANSPARENT_COL;
        eraserBtn_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::PERCENT;
        eraserBtn_layout.width.size = 0.35f;
        eraserBtn_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        eraserBtn_layout.child_alignment.horizontal = core::components::Layout::ChildAlignment::Start;
        eraserBtn_layout.padding.left = 12;
        eraserBtn_label->setParent(*eraserBtn);
        eraserBtn->setParent(*pencilContainer);


        auto fileExplorerContainer = core::ecs::EntityRegistry::Create<core::ecs::entities::UIRect>("Entity::UI::UIRect");
        auto &fileExplorerContainer_rect = fileExplorerContainer->getComponentMutable<core::components::UIRect>();
        auto &fileExplorerContainer_layout = fileExplorerContainer->getComponentMutable<core::components::Layout>();

        fileExplorerContainer_rect.color = core::types::Color::TRANSPARENT_COL;
        fileExplorerContainer_rect.corner_radius.top_left = 4.0f;
        fileExplorerContainer_rect.corner_radius.top_right = 4.0f;
        fileExplorerContainer_rect.corner_radius.bottom_left = 4.0f;
        fileExplorerContainer_rect.corner_radius.bottom_right = 4.0f;
        fileExplorerContainer_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        fileExplorerContainer_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::PERCENT;
        fileExplorerContainer_layout.height.size = 0.30f;
        fileExplorerContainer_layout.padding.left = 16;
        fileExplorerContainer_layout.padding.right = 16;
        fileExplorerContainer_layout.padding.top = 16;
        fileExplorerContainer_layout.padding.bottom = 16;
        fileExplorerContainer->setParent(*option_panel);

        auto fileExplorer = core::ecs::EntityRegistry::Create<core::ecs::entities::UIFileExplorer>("Entity::UI::UIRect::UIFileExplorer");
        fileExplorer->setRootPath(project::ProjectManager::GetCurrentProjectPath().string());
        fileExplorer->setParent(*fileExplorerContainer);


        auto colorPickContainer = core::ecs::EntityRegistry::Create<core::ecs::entities::UIRect>("Entity::UI::UIRect");
        auto &colorPickContaine_rect = colorPickContainer->getComponentMutable<core::components::UIRect>();
        auto &colorPickContainer_layout = colorPickContainer->getComponentMutable<core::components::Layout>();

        colorPickContaine_rect.color = core::types::Color::TRANSPARENT_COL;
        colorPickContaine_rect.corner_radius.top_left = 4.0f;
        colorPickContaine_rect.corner_radius.top_right = 4.0f;
        colorPickContaine_rect.corner_radius.bottom_left = 4.0f;
        colorPickContaine_rect.corner_radius.bottom_right = 4.0f;
        colorPickContainer_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        colorPickContainer_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        colorPickContainer_layout.padding.left = 16;
        colorPickContainer_layout.padding.right = 16;
        colorPickContainer_layout.padding.top = 16;
        colorPickContainer_layout.padding.bottom = 16;
        colorPickContainer->setParent(*option_panel);

        auto colorPicker = core::ecs::EntityRegistry::Create<core::ecs::entities::UIColorPicker>("Entity::UI::UIRect::UIColorPicker");
        colorPicker->setParent(*colorPickContainer);

        auto canvas_container = core::ecs::EntityRegistry::Create<core::ecs::entities::UI>("Entity::UI");
        auto &canvas_container_layout = canvas_container->getComponentMutable<core::components::Layout>();
        canvas_container_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        canvas_container_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        canvas_container_layout.child_alignment.vertical = core::components::Layout::ChildAlignment::Start;
        canvas_container_layout.direction = core::components::Layout::Direction::Vertical;
        canvas_container_layout.child_gap = 8;
        canvas_container->setParent(*texture_editor_container);

        auto canvas_area = core::ecs::EntityRegistry::Create<core::ecs::entities::UI>("Entity::UI");
        auto &canvas_area_layout = canvas_area->getComponentMutable<core::components::Layout>();
        canvas_area_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        canvas_area_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        canvas_area_layout.child_alignment.vertical = core::components::Layout::ChildAlignment::Start;
        canvas_area->setParent(*canvas_container);

        auto bottom_panel = core::ecs::EntityRegistry::Create<core::ecs::entities::UIRect>("Entity::UI::UIRect");
        auto &bottom_panel_rect = bottom_panel->getComponentMutable<core::components::UIRect>();
        auto &bottom_panel_layout = bottom_panel->getComponentMutable<core::components::Layout>();

        bottom_panel_rect.color = core::types::Color::WHITE;
        bottom_panel_rect.corner_radius.top_left = 4.0f;
        bottom_panel_rect.corner_radius.top_right = 4.0f;
        bottom_panel_rect.corner_radius.bottom_left = 4.0f;
        bottom_panel_rect.corner_radius.bottom_right = 4.0f;
        bottom_panel_layout.direction = core::components::Layout::Direction::Vertical;
        bottom_panel_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        bottom_panel_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::PERCENT;
        bottom_panel_layout.height.size = 0.30f;
        bottom_panel_layout.padding = { 16, 16, 16, 16 };
        bottom_panel_layout.child_gap = 8;
        bottom_panel->setParent(*canvas_container);

        auto timelineControlsRow = core::ecs::EntityRegistry::Create<core::ecs::entities::UI>("Entity::UI");
        auto &timelineControlsRow_layout = timelineControlsRow->getComponentMutable<core::components::Layout>();
        timelineControlsRow_layout.direction = core::components::Layout::Direction::Horizontal;
        timelineControlsRow_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        timelineControlsRow_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::FIT;
        timelineControlsRow_layout.child_alignment.vertical = core::components::Layout::ChildAlignment::Center;
        timelineControlsRow_layout.child_gap = 8;
        timelineControlsRow->setParent(*bottom_panel);

        auto timelineControlsSpacer = core::ecs::EntityRegistry::Create<core::ecs::entities::UI>("Entity::UI");
        auto &timelineControlsSpacer_layout = timelineControlsSpacer->getComponentMutable<core::components::Layout>();
        timelineControlsSpacer_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        timelineControlsSpacer->setParent(*timelineControlsRow);

        auto animationControlButtonContainer = core::ecs::EntityRegistry::Create<core::ecs::entities::UI>("Entity::UI");
        auto &animationControlButtonContainer_layout = animationControlButtonContainer->getComponentMutable<core::components::Layout>();
        animationControlButtonContainer_layout.direction = core::components::Layout::Direction::Horizontal;
        animationControlButtonContainer_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::FIT;
        animationControlButtonContainer_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::FIT;
        animationControlButtonContainer_layout.child_gap = 8;
        animationControlButtonContainer->setParent(*timelineControlsRow);

        auto makePlaybackBtn = [&animationControlButtonContainer](const std::string &text) {
            auto btn = core::ecs::EntityRegistry::Create<core::ecs::entities::UIButton>("Entity::UI::UIRect::UIButton");
            auto btn_label = core::ecs::EntityRegistry::Create<core::ecs::entities::UILabel>("Entity::UI::UILabel");
            auto &btn_rect = btn->getComponentMutable<core::components::UIRect>();
            auto &btn_layout = btn->getComponentMutable<core::components::Layout>();

            btn_label->setText(text);
            btn_label->getComponentMutable<core::components::UI>().modulate = core::types::Color::BLACK;
            btn_label->setFontSize(12);
            btn_rect.color = core::types::Color::TRANSPARENT_COL;
            btn_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::FIT;
            btn_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::FIT;
            btn_layout.padding.left = 12;
            btn_layout.padding.right = 12;
            btn_label->setParent(*btn);
            btn->setParent(*animationControlButtonContainer);
            return btn;
        };

        auto previousFrameBtn = makePlaybackBtn("Previous");
        auto playBtn = makePlaybackBtn("Play");
        auto pauseBtn = makePlaybackBtn("Pause");
        auto nextFrameBtn = makePlaybackBtn("Next");

        auto fpsLabel = core::ecs::EntityRegistry::Create<core::ecs::entities::UILabel>("Entity::UI::UILabel");
        fpsLabel->setText("FPS");
        fpsLabel->setFontSize(12);
        fpsLabel->getComponentMutable<core::components::UI>().modulate = core::types::Color::BLACK;
        fpsLabel->setParent(*animationControlButtonContainer);

        auto fpsNumberInput = core::ecs::EntityRegistry::Create<core::ecs::entities::UINumberInput>("Entity::UI::UIInput::UINumberInput");
        fpsNumberInput->getComponentMutable<core::components::UIInput>().input_type = atmo::core::components::UIInput::InputType::Int;
        fpsNumberInput->getComponentMutable<core::components::UINumberInput>().value = m_fps;
        fpsNumberInput->setParent(*animationControlButtonContainer);

        fpsNumberInput->getSignal<int>("IntValueChanged").connect([this](int val) {
            if (val < 1 || val > MAX_FPS) {
                spdlog::warn("FPS is not inside 1 - {} bounds, clamped", MAX_FPS);
                val = common::math::Clamp(val, 1, MAX_FPS);
            }
            m_fps = val;
        });

        playBtn->getSignal<>("Pressed").connect([this]() {
            setPlaying(true);
            m_frame_time_accumulator = 0.0f;
        });
        pauseBtn->getSignal<>("Pressed").connect([this]() { setPlaying(false); });
        previousFrameBtn->getSignal<>("Pressed").connect([this]() {
            setPlaying(false);
            stepFrame(-1);
        });
        nextFrameBtn->getSignal<>("Pressed").connect([this]() {
            setPlaying(false);
            stepFrame(1);
        });

        auto frameBtnContainer = core::ecs::EntityRegistry::Create<core::ecs::entities::UI>("Entity::UI");
        auto &frameBtnContainer_layout = frameBtnContainer->getComponentMutable<core::components::Layout>();
        frameBtnContainer_layout.direction = core::components::Layout::Direction::Horizontal;
        frameBtnContainer_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        frameBtnContainer_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::FIT;
        frameBtnContainer_layout.child_alignment.horizontal = core::components::Layout::ChildAlignment::End;
        frameBtnContainer_layout.child_gap = 8;
        frameBtnContainer->setParent(*timelineControlsRow);

        auto addFrameBtn = core::ecs::EntityRegistry::Create<core::ecs::entities::UIButton>("Entity::UI::UIRect::UIButton");
        auto addFrameBtn_label = core::ecs::EntityRegistry::Create<core::ecs::entities::UILabel>("Entity::UI::UILabel");
        auto &addFrameBtn_rect = addFrameBtn->getComponentMutable<core::components::UIRect>();
        auto &addFrameBtn_layout = addFrameBtn->getComponentMutable<core::components::Layout>();
        addFrameBtn->setParent(*frameBtnContainer);

        addFrameBtn_label->setText("Add Frame");
        addFrameBtn_label->getComponentMutable<core::components::UI>().modulate = core::types::Color::BLACK;
        addFrameBtn_label->setFontSize(12);
        addFrameBtn_rect.color = core::types::Color::TRANSPARENT_COL;
        addFrameBtn_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::FIT;
        addFrameBtn_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::FIT;
        addFrameBtn_layout.padding.left = 12;
        addFrameBtn_layout.padding.right = 12;
        addFrameBtn_label->setParent(*addFrameBtn);

        auto deleteFrameBtn = core::ecs::EntityRegistry::Create<core::ecs::entities::UIButton>("Entity::UI::UIRect::UIButton");
        auto deleteFrameBtn_label = core::ecs::EntityRegistry::Create<core::ecs::entities::UILabel>("Entity::UI::UILabel");
        auto &deleteFrameBtn_rect = deleteFrameBtn->getComponentMutable<core::components::UIRect>();
        auto &deleteFrameBtn_layout = deleteFrameBtn->getComponentMutable<core::components::Layout>();
        deleteFrameBtn->setParent(*frameBtnContainer);

        deleteFrameBtn_label->setText("Delete Frame");
        deleteFrameBtn_label->getComponentMutable<core::components::UI>().modulate = core::types::Color::BLACK;
        deleteFrameBtn_label->setFontSize(12);
        deleteFrameBtn_rect.color = core::types::Color::TRANSPARENT_COL;
        deleteFrameBtn_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::FIT;
        deleteFrameBtn_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::FIT;
        deleteFrameBtn_layout.padding.left = 12;
        deleteFrameBtn_layout.padding.right = 12;
        deleteFrameBtn_label->setParent(*deleteFrameBtn);

        auto timeline = core::ecs::EntityRegistry::Create<core::ecs::entities::UIGrid>("Entity::UI::UIRect::UIGrid");
        m_timeline_handle = timeline->getHandle();
        timeline->setParent(*bottom_panel);

        auto canvas =core::ecs::EntityRegistry::Create<core::ecs::entities::UIDrawingCanvas>("Entity::UI::UIDrawingCanvas");
        auto &canvas_layout = canvas->getComponentMutable<core::components::Layout>();
        canvas_layout.z_index = 0;

        auto &canvasInfo = canvas->getComponentMutable<core::components::UIDrawingCanvas>();
        canvasInfo.canvas_size = { 1.0f, 1.0f };
        canvasInfo.zoom = 1.0f;
        canvasInfo.offset = { 0.0f, 0.0f };
        canvas->setParent(*canvas_area);

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
        canvas->initPixelBuffer(core::ecs::entities::DEFAULT_TEXTURE_SIZE, core::ecs::entities::DEFAULT_TEXTURE_SIZE);

        timeline->getSignal<int, int>("CellSelected").connect([canvasHandle = canvas->getHandle()](int layerIdx, int frameIdx) {
            if (!canvasHandle.is_alive() || layerIdx < 0 || frameIdx < 0) {
                return;
            }
            core::ecs::entities::UIDrawingCanvas canvas(core::ecs::EntityRegistry::GetEntityFromId(canvasHandle));
            auto &canvas_comp = canvas.getComponentMutable<core::components::UIDrawingCanvas>();
            const auto nameList = canvas_comp.image.layerNames();

            if (layerIdx >= (int)nameList.size()) {
                return;
            }

            canvas_comp.image.selectLayer(nameList[layerIdx]);
            canvas_comp.image.selectFrame(static_cast<std::uint8_t>(frameIdx));
            canvas_comp.texture_dirty = true;
        });

        timeline->setRowBuilder([this](core::ecs::entities::Entity &leading, core::ecs::entities::Entity &actions, int row) {
            if (!m_canvas_handle.is_alive()) {
                return;
            }
            core::ecs::entities::UIDrawingCanvas canvas(core::ecs::EntityRegistry::GetEntityFromId(m_canvas_handle));
            const auto &image = canvas.getComponentMutable<core::components::UIDrawingCanvas>().image;
            const auto names = image.layerNames();
            if (row >= (int)names.size()) {
                return;
            }
            const std::string name = names[row];

            auto visibleBtn = core::ecs::EntityRegistry::Create<core::ecs::entities::UIButton>("Entity::UI::UIRect::UIButton");
            auto visibleBtn_label = core::ecs::EntityRegistry::Create<core::ecs::entities::UILabel>("Entity::UI::UILabel");
            auto &visibleBtn_rect = visibleBtn->getComponentMutable<core::components::UIRect>();
            auto &visibleBtn_layout = visibleBtn->getComponentMutable<core::components::Layout>();

            visibleBtn_label->setText(image.isLayerVisible(name) ? "Hide" : "Show");
            visibleBtn_label->getComponentMutable<core::components::UI>().modulate = core::types::Color::BLACK;
            visibleBtn_label->setFontSize(12);
            visibleBtn_rect.color = core::types::Color::TRANSPARENT_COL;
            visibleBtn_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::FIXED;
            visibleBtn_layout.width.size = core::components::Layout::SizingAxis::MinMax{ 48.0f, 48.0f };
            visibleBtn_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
            visibleBtn_label->setParent(*visibleBtn);
            visibleBtn->setParent(leading);

            auto labelHandle = visibleBtn_label->getHandle();
            if (row >= (int)m_visibility_label_handles.size())
                m_visibility_label_handles.resize(row + 1);
            m_visibility_label_handles[row] = labelHandle;
            visibleBtn->getSignal<>("Pressed").connect([this, name, labelHandle]() {
                if (!m_canvas_handle.is_alive() || !labelHandle.is_alive()) {
                    return;
                }
                core::ecs::entities::UIDrawingCanvas canvas(core::ecs::EntityRegistry::GetEntityFromId(m_canvas_handle));
                auto &canvas_comp = canvas.getComponentMutable<core::components::UIDrawingCanvas>();

                bool visible = !canvas_comp.image.isLayerVisible(name);
                canvas_comp.image.setLayerVisible(name, visible);
                canvas_comp.texture_dirty = true;

                core::ecs::entities::UILabel label(core::ecs::EntityRegistry::GetEntityFromId(labelHandle));
                label.setText(visible ? "Hide" : "Show");

                refreshAllLayerVisible();
            });

            auto deleteBtn = core::ecs::EntityRegistry::Create<core::ecs::entities::UIButton>("Entity::UI::UIRect::UIButton");
            auto deleteBtn_label = core::ecs::EntityRegistry::Create<core::ecs::entities::UILabel>("Entity::UI::UILabel");
            auto &deleteBtn_rect = deleteBtn->getComponentMutable<core::components::UIRect>();
            auto &deleteBtn_layout = deleteBtn->getComponentMutable<core::components::Layout>();

            deleteBtn_label->setText("-");
            deleteBtn_label->getComponentMutable<core::components::UI>().modulate = core::types::Color::BLACK;
            deleteBtn_label->setFontSize(12);
            deleteBtn_rect.color = core::types::Color::TRANSPARENT_COL;
            deleteBtn_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::FIT;
            deleteBtn_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
            deleteBtn_layout.padding.left = 8;
            deleteBtn_layout.padding.right = 8;
            deleteBtn_label->setParent(*deleteBtn);
            deleteBtn->setParent(actions);

            deleteBtn->getSignal<>("Pressed").connect([this, name]() {
                core::SignalQueue::Enqueue([this, name]() {
                    if (!m_canvas_handle.is_alive()) {
                        return;
                    }
                    core::ecs::entities::UIDrawingCanvas canvas(core::ecs::EntityRegistry::GetEntityFromId(m_canvas_handle));
                    auto &canvas_comp = canvas.getComponentMutable<core::components::UIDrawingCanvas>();

                    if (canvas_comp.image.layerNames().size() <= 1) {
                        return;
                    }

                    canvas_comp.image.removeLayer(name);
                    canvas_comp.texture_dirty = true;
                    refreshTimeline();
                });
            });
        });

        auto moveSelectedLayer = [this](int step) {
            if (!m_canvas_handle.is_alive()) {
                return;
            }
            core::ecs::entities::UIDrawingCanvas canvas(core::ecs::EntityRegistry::GetEntityFromId(m_canvas_handle));
            auto &canvas_comp = canvas.getComponentMutable<core::components::UIDrawingCanvas>();
            const auto names = canvas_comp.image.layerNames();
            const int from = canvas_comp.image.currentLayer();
            const int to = from + step;

            if (from >= (int)names.size() || to < 0 || to >= (int)names.size()) {
                return;
            }

            canvas_comp.image.moveLayer(names[from], static_cast<std::uint8_t>(to));
            canvas_comp.texture_dirty = true;
            refreshTimeline();
        };

        timeline->setHeaderBuilder([this, moveSelectedLayer](core::ecs::entities::Entity &corner) {
            auto toggleAllBtn = core::ecs::EntityRegistry::Create<core::ecs::entities::UIButton>("Entity::UI::UIRect::UIButton");
            auto toggleAllBtn_label = core::ecs::EntityRegistry::Create<core::ecs::entities::UILabel>("Entity::UI::UILabel");
            auto &toggleAllBtn_rect = toggleAllBtn->getComponentMutable<core::components::UIRect>();
            auto &toggleAllBtn_layout = toggleAllBtn->getComponentMutable<core::components::Layout>();

            toggleAllBtn_label->setText(m_allLayerVisible ? "Hide" : "Show");
            m_toggle_all_label_handle = toggleAllBtn_label->getHandle();
            toggleAllBtn_label->getComponentMutable<core::components::UI>().modulate = core::types::Color::BLACK;
            toggleAllBtn_label->setFontSize(12);
            toggleAllBtn_rect.color = core::types::Color::TRANSPARENT_COL;
            toggleAllBtn_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::FIXED;
            toggleAllBtn_layout.width.size = core::components::Layout::SizingAxis::MinMax{ 48.0f, 48.0f };
            toggleAllBtn_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
            toggleAllBtn_label->setParent(*toggleAllBtn);
            toggleAllBtn->setParent(corner);

            toggleAllBtn->getSignal<>("Pressed").connect([this]() {
                if (!m_canvas_handle.is_alive()) {
                    return;
                }
                core::ecs::entities::UIDrawingCanvas canvas(core::ecs::EntityRegistry::GetEntityFromId(m_canvas_handle));
                auto &canvas_comp = canvas.getComponentMutable<core::components::UIDrawingCanvas>();
                const auto names = canvas_comp.image.layerNames();

                for (const auto &name : names)
                    canvas_comp.image.setLayerVisible(name, !m_allLayerVisible);
                canvas_comp.texture_dirty = true;

                for (std::size_t i = 0; i < names.size() && i < m_visibility_label_handles.size(); i++) {
                    if (!m_visibility_label_handles[i].is_alive())
                        continue;
                    core::ecs::entities::UILabel label(core::ecs::EntityRegistry::GetEntityFromId(m_visibility_label_handles[i]));
                    label.setText(canvas_comp.image.isLayerVisible(names[i]) ? "Hide" : "Show");
                }

                refreshAllLayerVisible();
            });

            auto layerUpBtn = core::ecs::EntityRegistry::Create<core::ecs::entities::UIButton>("Entity::UI::UIRect::UIButton");
            auto layerUpBtn_label = core::ecs::EntityRegistry::Create<core::ecs::entities::UILabel>("Entity::UI::UILabel");
            auto &layerUpBtn_rect = layerUpBtn->getComponentMutable<core::components::UIRect>();
            auto &layerUpBtn_layout = layerUpBtn->getComponentMutable<core::components::Layout>();

            layerUpBtn_label->setText("^");
            layerUpBtn_label->getComponentMutable<core::components::UI>().modulate = core::types::Color::BLACK;
            layerUpBtn_label->setFontSize(12);
            layerUpBtn_rect.color = core::types::Color::TRANSPARENT_COL;
            layerUpBtn_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::FIT;
            layerUpBtn_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
            layerUpBtn_layout.padding.left = 8;
            layerUpBtn_layout.padding.right = 8;
            layerUpBtn_label->setParent(*layerUpBtn);
            layerUpBtn->setParent(corner);

            auto layerDownBtn = core::ecs::EntityRegistry::Create<core::ecs::entities::UIButton>("Entity::UI::UIRect::UIButton");
            auto layerDownBtn_label = core::ecs::EntityRegistry::Create<core::ecs::entities::UILabel>("Entity::UI::UILabel");
            auto &layerDownBtn_rect = layerDownBtn->getComponentMutable<core::components::UIRect>();
            auto &layerDownBtn_layout = layerDownBtn->getComponentMutable<core::components::Layout>();

            layerDownBtn_label->setText("v");
            layerDownBtn_label->getComponentMutable<core::components::UI>().modulate = core::types::Color::BLACK;
            layerDownBtn_label->setFontSize(12);
            layerDownBtn_rect.color = core::types::Color::TRANSPARENT_COL;
            layerDownBtn_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::FIT;
            layerDownBtn_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
            layerDownBtn_layout.padding.left = 8;
            layerDownBtn_layout.padding.right = 8;
            layerDownBtn_label->setParent(*layerDownBtn);
            layerDownBtn->setParent(corner);

            layerUpBtn->getSignal<>("Pressed").connect([moveSelectedLayer]() { core::SignalQueue::Enqueue([moveSelectedLayer]() { moveSelectedLayer(-1); }); });
            layerDownBtn->getSignal<>("Pressed").connect([moveSelectedLayer]() { core::SignalQueue::Enqueue([moveSelectedLayer]() { moveSelectedLayer(1); }); });

            auto spacer = core::ecs::EntityRegistry::Create<core::ecs::entities::UI>("Entity::UI");
            spacer->getComponentMutable<core::components::Layout>().width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
            spacer->setParent(corner);

            auto addLayerBtn = core::ecs::EntityRegistry::Create<core::ecs::entities::UIButton>("Entity::UI::UIRect::UIButton");
            auto addLayerBtn_label = core::ecs::EntityRegistry::Create<core::ecs::entities::UILabel>("Entity::UI::UILabel");
            auto &addLayerBtn_rect = addLayerBtn->getComponentMutable<core::components::UIRect>();
            auto &addLayerBtn_layout = addLayerBtn->getComponentMutable<core::components::Layout>();

            addLayerBtn_label->setText("+");
            addLayerBtn_label->getComponentMutable<core::components::UI>().modulate = core::types::Color::BLACK;
            addLayerBtn_label->setFontSize(12);
            addLayerBtn_rect.color = core::types::Color::TRANSPARENT_COL;
            addLayerBtn_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::FIT;
            addLayerBtn_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
            addLayerBtn_layout.padding.left = 8;
            addLayerBtn_layout.padding.right = 8;
            addLayerBtn_label->setParent(*addLayerBtn);
            addLayerBtn->setParent(corner);

            addLayerBtn->getSignal<>("Pressed").connect([this]() {
                core::SignalQueue::Enqueue([this]() {
                    if (!m_canvas_handle.is_alive()) {
                        return;
                    }
                    core::ecs::entities::UIDrawingCanvas canvas(core::ecs::EntityRegistry::GetEntityFromId(m_canvas_handle));
                    auto &image = canvas.getComponentMutable<core::components::UIDrawingCanvas>().image;
                    const auto names = image.layerNames();

                    std::size_t nb = names.size() + 1;
                    std::string name = std::format("Layer {}", nb);
                    while (std::find(names.begin(), names.end(), name) != names.end())
                        name = std::format("Layer {}", ++nb);

                    image.addLayer(name);
                    refreshTimeline();
                });
            });
        });


        auto canvasHandle = canvas->getHandle();
        m_canvas_handle = canvasHandle;
        refreshTimeline();

        widthNumberInput->getSignal<int>("IntValueChanged").connect([canvasHandle](int val) {
            if (!canvasHandle.is_alive()) {
                return;
            }
            if (val < 1 || val > core::ecs::entities::MAX_FRAME_SIZE) {
                spdlog::warn("Width is not inside 1 - {} bounds, clamped", core::ecs::entities::MAX_FRAME_SIZE);
                val = common::math::Clamp(val, 1, core::ecs::entities::MAX_FRAME_SIZE);
            }

            core::ecs::entities::UIDrawingCanvas canvas(core::ecs::EntityRegistry::GetEntityFromId(canvasHandle));
            auto &comp = canvas.getComponentMutable<core::components::UIDrawingCanvas>();

            canvas.resizeCanvas(val, comp.texture_size.y);
        });

        heightNumberInput->getSignal<int>("IntValueChanged").connect([canvasHandle](int val) {
            if (!canvasHandle.is_alive()) {
                return;
            }
            if (val < 1 || val > core::ecs::entities::MAX_FRAME_SIZE) {
                spdlog::warn("Height is not inside 1 - {} bounds, clamped", core::ecs::entities::MAX_FRAME_SIZE);
                val = common::math::Clamp(val, 1, core::ecs::entities::MAX_FRAME_SIZE);
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

        previewBtn->getSignal<>("Pressed").connect([canvasHandle, this]() {
            if (!canvasHandle.is_alive() || atmo::project::ProjectManager::GetSettings().app.default_scene.empty()) {
                return;
            }

            m_scene_ctx->loadSceneFromFile(project::FileSystem::ResolvePath(atmo::project::ProjectManager::GetSettings().app.default_scene).string());

            core::ecs::entities::UIDrawingCanvas canvas(core::ecs::EntityRegistry::GetEntityFromId(canvasHandle));
            auto &canvas_comp = canvas.getComponentMutable<core::components::UIDrawingCanvas>();

            canvas_comp.preview = !canvas_comp.preview;
            canvas_comp.zoom = 1.0f;
            canvas_comp.offset = { 0.0f, 0.0f };
        });

        pencilBtn->getSignal<bool>("Toggle").connect([canvasHandle](bool new_state) {
            if (!canvasHandle.is_alive()) {
                return;
            }
            core::ecs::entities::UIDrawingCanvas canvas(core::ecs::EntityRegistry::GetEntityFromId(canvasHandle));
            auto &canvas_comp = canvas.getComponentMutable<core::components::UIDrawingCanvas>();

            canvas_comp.pen = core::components::UIDrawingCanvas::DrawType::PENCIL;
        });

        eraserBtn->getSignal<bool>("Toggle").connect([canvasHandle](bool new_state) {
            if (!canvasHandle.is_alive()) {
                return;
            }
            core::ecs::entities::UIDrawingCanvas canvas(core::ecs::EntityRegistry::GetEntityFromId(canvasHandle));
            auto &canvas_comp = canvas.getComponentMutable<core::components::UIDrawingCanvas>();

            canvas_comp.pen = core::components::UIDrawingCanvas::DrawType::ERASER;
        });

        fileExplorer->getSignal<std::string>("FileFocus").connect([this](std::string path) { open(path); });

        saveBtn->getSignal<>("Pressed").connect([this]() { save(); });

        exportBtn->getSignal<>("Released").connect([this]() { openExportPopup(); });




        addFrameBtn->getSignal<>("Pressed").connect([this]() {
            if (!m_canvas_handle.is_alive()) {
                return;
            }
            core::ecs::entities::UIDrawingCanvas canvas(core::ecs::EntityRegistry::GetEntityFromId(m_canvas_handle));
            auto &canvas_comp = canvas.getComponentMutable<core::components::UIDrawingCanvas>();

            canvas_comp.image.addFrame();
            canvas_comp.image.selectFrame(static_cast<std::uint8_t>(canvas_comp.image.frameCount() - 1));
            canvas_comp.texture_dirty = true;
            refreshTimeline();
        });

        deleteFrameBtn->getSignal<>("Pressed").connect([this]() {
            if (!m_canvas_handle.is_alive()) {
                return;
            }
            core::ecs::entities::UIDrawingCanvas canvas(core::ecs::EntityRegistry::GetEntityFromId(m_canvas_handle));
            auto &canvas_comp = canvas.getComponentMutable<core::components::UIDrawingCanvas>();

            if (canvas_comp.image.frameCount() <= 1) {
                return;
            }

            canvas_comp.image.removeFrame(canvas_comp.image.currentFrameIndex());
            canvas_comp.texture_dirty = true;
            refreshTimeline();
        });
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
        const std::string &path = *p_file_path;

        std::string ext = std::filesystem::path(path).extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        if (ext == ".atmo") {
            canvas.importCanvas(path);
            refreshTimeline();
            return;
        }

        core::types::Vector2i sheetSize = { 0, 0 };
        try {
            sheetSize = core::ecs::entities::UIDrawingCanvas::ProbeImageSize(path);
        } catch (const core::ecs::entities::UIDrawingCanvas::ImportException &) {
            canvas.importCanvas(path);
            refreshTimeline();
            return;
        }

        openSpriteSheetImportPopup(path, sheetSize);
    }

    void TextureEditor::openSpriteSheetImportPopup(const std::string &path, core::types::Vector2i sheetSize)
    {
        if (!m_root_handle.is_alive())
            return;

        core::ecs::entities::UI root(core::ecs::EntityRegistry::GetEntityFromId(m_root_handle));

        auto popup = core::ecs::EntityRegistry::Create<core::ecs::entities::UIPopup>("Entity::UI::UIRect::UIPopup");
        popup->setParent(root);

        auto popup_bg = core::ecs::EntityRegistry::Create<core::ecs::entities::UIRect>("Entity::UI::UIRect");
        auto &popup_bg_rect = popup_bg->getComponentMutable<core::components::UIRect>();
        auto &popup_bg_layout = popup_bg->getComponentMutable<core::components::Layout>();
        popup_bg_rect.color = core::types::Color::WHITE;
        popup_bg_rect.corner_radius = { 4.0f, 4.0f, 4.0f, 4.0f };
        popup_bg_layout.direction = core::components::Layout::Direction::Vertical;
        popup_bg_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::FIXED;
        popup_bg_layout.width.size = core::components::Layout::SizingAxis::MinMax{ 320.0f, 320.0f };
        popup_bg_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::FIT;
        popup_bg_layout.padding = { 16, 16, 16, 16 };
        popup_bg_layout.child_gap = 12;
        popup_bg->setParent(*popup);

        auto makeLabel = [](const std::string &text, int fontSize) {
            auto label = core::ecs::EntityRegistry::Create<core::ecs::entities::UILabel>("Entity::UI::UILabel");
            label->setText(text);
            label->setFontSize(fontSize);
            label->getComponentMutable<core::components::UI>().modulate = core::types::Color::BLACK;
            return label;
        };

        makeLabel("Import sprite sheet", 14)->setParent(*popup_bg);
        makeLabel(std::format("{}: {}x{} px", std::filesystem::path(path).filename().string(), sheetSize.x, sheetSize.y), 12)->setParent(*popup_bg);

        auto makeIntField = [&popup_bg, &makeLabel](const std::string &text, int initialValue) {
            auto row = core::ecs::EntityRegistry::Create<core::ecs::entities::UI>("Entity::UI");
            auto &row_layout = row->getComponentMutable<core::components::Layout>();
            row_layout.direction = core::components::Layout::Direction::Horizontal;
            row_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
            row_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::FIT;
            row_layout.child_alignment.vertical = core::components::Layout::ChildAlignment::Center;
            row_layout.child_gap = 8;
            row->setParent(*popup_bg);

            auto label = makeLabel(text, 12);
            label->getComponentMutable<core::components::Layout>().width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
            label->setParent(*row);

            auto input = core::ecs::EntityRegistry::Create<core::ecs::entities::UINumberInput>("Entity::UI::UIInput::UINumberInput");
            input->getComponentMutable<core::components::UIInput>().input_type = atmo::core::components::UIInput::InputType::Int;
            input->getComponentMutable<core::components::UINumberInput>().value = initialValue;
            input->setParent(*row);
            return input->getHandle();
        };

        auto frameCountHandle = makeIntField("Frame count", 1);
        auto frameWidthHandle = makeIntField("Frame width", sheetSize.x);
        auto frameHeightHandle = makeIntField("Frame height", sheetSize.y);

        auto errorLabel = makeLabel("", 12);
        errorLabel->getComponentMutable<core::components::UI>().modulate = core::types::Color::RED;
        errorLabel->setParent(*popup_bg);

        auto btnRow = core::ecs::EntityRegistry::Create<core::ecs::entities::UI>("Entity::UI");
        auto &btnRow_layout = btnRow->getComponentMutable<core::components::Layout>();
        btnRow_layout.direction = core::components::Layout::Direction::Horizontal;
        btnRow_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        btnRow_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::FIT;
        btnRow_layout.child_alignment.horizontal = core::components::Layout::ChildAlignment::End;
        btnRow_layout.child_gap = 8;
        btnRow->setParent(*popup_bg);

        auto makeTextBtn = [&btnRow, &makeLabel](const std::string &text) {
            auto btn = core::ecs::EntityRegistry::Create<core::ecs::entities::UIButton>("Entity::UI::UIRect::UIButton");
            auto &btn_layout = btn->getComponentMutable<core::components::Layout>();
            btn->getComponentMutable<core::components::UIRect>().color = core::types::Color::TRANSPARENT_COL;
            btn_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::FIT;
            btn_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::FIT;
            btn_layout.padding.left = 12;
            btn_layout.padding.right = 12;
            makeLabel(text, 12)->setParent(*btn);
            btn->setParent(*btnRow);
            return btn;
        };

        auto cancelBtn = makeTextBtn("Cancel");
        cancelBtn->getSignal<>("Released").connect([popup]() { popup->destroy(); });

        auto importBtn = makeTextBtn("Import");
        importBtn->getSignal<>("Released").connect(
            [this, path, popup, frameCountHandle, frameWidthHandle, frameHeightHandle, errorLabelHandle = errorLabel->getHandle()]() {
                if (!m_canvas_handle.is_alive() || !frameCountHandle.is_alive() || !frameWidthHandle.is_alive() || !frameHeightHandle.is_alive())
                    return;

                auto readInt = [](flecs::entity handle) {
                    core::ecs::entities::UINumberInput input(core::ecs::EntityRegistry::GetEntityFromId(handle));
                    return std::visit([](auto v) { return static_cast<int>(v); }, input.getComponent<core::components::UINumberInput>().value);
                };

                core::ecs::entities::UIDrawingCanvas canvas(core::ecs::EntityRegistry::GetEntityFromId(m_canvas_handle));
                try {
                    canvas.importSpriteSheet(path, readInt(frameWidthHandle), readInt(frameHeightHandle), readInt(frameCountHandle));
                } catch (const core::ecs::entities::UIDrawingCanvas::ImportException &e) {
                    if (errorLabelHandle.is_alive()) {
                        core::ecs::entities::UILabel errorLabel(core::ecs::EntityRegistry::GetEntityFromId(errorLabelHandle));
                        errorLabel.setText(e.getReason());
                    }
                    return;
                }

                refreshTimeline();
                popup->destroy();
            });
    }

    void TextureEditor::openExportPopup()
    {
        if (!m_root_handle.is_alive() || !m_canvas_handle.is_alive())
            return;

        constexpr int FORMAT_BUTTON_GROUP = 6;
        static constexpr std::string_view ATMO_EXTENSION = ".atmo";
        static const std::vector<std::string> extensions = { ".atmo", ".png", ".bmp", ".jpg" };

        core::ecs::entities::UI root(core::ecs::EntityRegistry::GetEntityFromId(m_root_handle));
        core::ecs::entities::UIDrawingCanvas canvas(core::ecs::EntityRegistry::GetEntityFromId(m_canvas_handle));
        const auto &image = canvas.getComponentMutable<core::components::UIDrawingCanvas>().image;
        const auto layerNames = image.layerNames();

        std::filesystem::path exportDir = project::ProjectManager::GetCurrentProjectPath();
        std::string defaultName = "texture";
        if (p_file_path) {
            const auto opened = project::FileSystem::ResolvePath(*p_file_path);
            exportDir = opened.parent_path();
            defaultName = opened.stem().string();
        }

        auto selectedExt = std::make_shared<std::string>(".png");
        auto selectedLayers = std::make_shared<std::vector<bool>>();
        for (const auto &name : layerNames)
            selectedLayers->push_back(image.isLayerVisible(name));

        auto popup = core::ecs::EntityRegistry::Create<core::ecs::entities::UIPopup>("Entity::UI::UIRect::UIPopup");
        popup->setParent(root);

        auto popup_bg = core::ecs::EntityRegistry::Create<core::ecs::entities::UIRect>("Entity::UI::UIRect");
        auto &popup_bg_rect = popup_bg->getComponentMutable<core::components::UIRect>();
        auto &popup_bg_layout = popup_bg->getComponentMutable<core::components::Layout>();
        popup_bg_rect.color = core::types::Color::WHITE;
        popup_bg_rect.corner_radius = { 4.0f, 4.0f, 4.0f, 4.0f };
        popup_bg_layout.direction = core::components::Layout::Direction::Vertical;
        popup_bg_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::FIXED;
        popup_bg_layout.width.size = core::components::Layout::SizingAxis::MinMax{ 320.0f, 320.0f };
        popup_bg_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::FIT;
        popup_bg_layout.padding = { 16, 16, 16, 16 };
        popup_bg_layout.child_gap = 12;
        popup_bg->setParent(*popup);

        auto makeLabel = [](const std::string &text, int fontSize) {
            auto label = core::ecs::EntityRegistry::Create<core::ecs::entities::UILabel>("Entity::UI::UILabel");
            label->setText(text);
            label->setFontSize(fontSize);
            label->getComponentMutable<core::components::UI>().modulate = core::types::Color::BLACK;
            return label;
        };

        auto makeRow = [](core::ecs::entities::Entity &parent) {
            auto row = core::ecs::EntityRegistry::Create<core::ecs::entities::UI>("Entity::UI");
            auto &row_layout = row->getComponentMutable<core::components::Layout>();
            row_layout.direction = core::components::Layout::Direction::Horizontal;
            row_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
            row_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::FIT;
            row_layout.child_alignment.vertical = core::components::Layout::ChildAlignment::Center;
            row_layout.child_gap = 8;
            row->setParent(parent);
            return row;
        };

        auto makeTextBtn = [&makeLabel](core::ecs::entities::Entity &parent, const std::string &text) {
            auto btn = core::ecs::EntityRegistry::Create<core::ecs::entities::UIButton>("Entity::UI::UIRect::UIButton");
            auto &btn_layout = btn->getComponentMutable<core::components::Layout>();
            btn->getComponentMutable<core::components::UIRect>().color = core::types::Color::TRANSPARENT_COL;
            btn_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::FIT;
            btn_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::FIT;
            btn_layout.padding.left = 12;
            btn_layout.padding.right = 12;
            makeLabel(text, 12)->setParent(*btn);
            btn->setParent(parent);
            return btn;
        };

        makeLabel("Export texture", 14)->setParent(*popup_bg);

        auto nameRow = makeRow(*popup_bg);
        makeLabel("File name", 12)->setParent(*nameRow);
        auto nameInput = core::ecs::EntityRegistry::Create<core::ecs::entities::UITextInput>("Entity::UI::UIInput::UITextInput");
        nameInput->getComponentMutable<core::components::Layout>().width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        nameInput->setValue(defaultName);
        nameInput->setParent(*nameRow);

        auto formatRow = makeRow(*popup_bg);
        makeLabel("Format", 12)->setParent(*formatRow);

        auto layerSection = core::ecs::EntityRegistry::Create<core::ecs::entities::UI>("Entity::UI");
        auto &layerSection_layout = layerSection->getComponentMutable<core::components::Layout>();
        layerSection_layout.direction = core::components::Layout::Direction::Vertical;
        layerSection_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        layerSection_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::FIT;
        layerSection_layout.child_gap = 4;
        layerSection->setParent(*popup_bg);

        makeLabel("Layers to blend", 12)->setParent(*layerSection);
        for (std::size_t i = 0; i < layerNames.size(); i++) {
            auto layerRow = makeRow(*layerSection);

            auto checkbox = core::ecs::EntityRegistry::Create<core::ecs::entities::UICheckBox>("Entity::UI::UIRect::UIButton::UICheckBox");
            auto &checkbox_layout = checkbox->getComponentMutable<core::components::Layout>();
            checkbox_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::FIXED;
            checkbox_layout.width.size = core::components::Layout::SizingAxis::MinMax{ 20.0f, 20.0f };
            checkbox_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::FIXED;
            checkbox_layout.height.size = core::components::Layout::SizingAxis::MinMax{ 20.0f, 20.0f };
            checkbox->setParent(*layerRow);

            makeLabel(layerNames[i], 12)->setParent(*layerRow);

            checkbox->getSignal<bool>("Toggle").connect([selectedLayers, i](bool new_state) { (*selectedLayers)[i] = new_state; });

            const bool checked = (*selectedLayers)[i];
            checkbox->getComponentMutable<core::components::UIButton>().is_pressed = checked;
            checkbox->getSignal<bool>("Toggle").emit(checked);
        }

        for (const auto &ext : extensions) {
            auto formatBtn = makeTextBtn(*formatRow, ext);
            auto &formatBtn_comp = formatBtn->getComponentMutable<core::components::UIButton>();
            formatBtn_comp.toggle = true;
            formatBtn_comp.group = FORMAT_BUTTON_GROUP;

            formatBtn->getSignal<bool>("Toggle").connect([selectedExt, ext, layerSectionHandle = layerSection->getHandle()](bool new_state) {
                if (!new_state)
                    return;
                *selectedExt = ext;
                if (layerSectionHandle.is_alive()) {
                    core::ecs::entities::UI section(core::ecs::EntityRegistry::GetEntityFromId(layerSectionHandle));
                    section.getComponentMutable<core::components::UI>().visible = ext != ATMO_EXTENSION;
                }
            });

            if (ext == *selectedExt)
                formatBtn->press();
        }

        auto errorLabel = makeLabel("", 12);
        errorLabel->getComponentMutable<core::components::UI>().modulate = core::types::Color::RED;
        errorLabel->setParent(*popup_bg);

        auto btnRow = makeRow(*popup_bg);
        btnRow->getComponentMutable<core::components::Layout>().child_alignment.horizontal = core::components::Layout::ChildAlignment::End;

        auto cancelBtn = makeTextBtn(*btnRow, "Cancel");
        cancelBtn->getSignal<>("Released").connect([popup]() { popup->destroy(); });

        auto exportBtn = makeTextBtn(*btnRow, "Export");
        exportBtn->getSignal<>("Released").connect([this,
                                                    popup,
                                                    exportDir,
                                                    layerNames,
                                                    selectedExt,
                                                    selectedLayers,
                                                    nameInputHandle = nameInput->getHandle(),
                                                    errorLabelHandle = errorLabel->getHandle()]() {
            if (!m_canvas_handle.is_alive() || !nameInputHandle.is_alive())
                return;

            auto showError = [errorLabelHandle](const std::string &message) {
                if (!errorLabelHandle.is_alive())
                    return;
                core::ecs::entities::UILabel errorLabel(core::ecs::EntityRegistry::GetEntityFromId(errorLabelHandle));
                errorLabel.setText(message);
            };

            core::ecs::entities::UITextInput nameInput(core::ecs::EntityRegistry::GetEntityFromId(nameInputHandle));
            std::string fileName = nameInput.getComponent<core::components::UIInput>().input_data;
            if (fileName.empty() || fileName.find_first_of("/\\") != std::string::npos) {
                showError("Invalid file name");
                return;
            }
            if (std::filesystem::path(fileName).extension() != *selectedExt)
                fileName += *selectedExt;
            const std::string path = (exportDir / fileName).string();

            core::ecs::entities::UIDrawingCanvas canvas(core::ecs::EntityRegistry::GetEntityFromId(m_canvas_handle));
            auto &canvas_comp = canvas.getComponentMutable<core::components::UIDrawingCanvas>();

            bool exported = false;
            if (*selectedExt == ATMO_EXTENSION) {
                exported = canvas_comp.image.save(path);
            } else {
                std::vector<std::string> layersToBlend;
                for (std::size_t i = 0; i < layerNames.size(); i++)
                    if ((*selectedLayers)[i])
                        layersToBlend.push_back(layerNames[i]);

                if (layersToBlend.empty()) {
                    showError("Select at least one layer");
                    return;
                }
                exported = canvas.exportSpriteSheet(path, layersToBlend);
            }

            if (!exported) {
                showError(std::format("Could not write {}", fileName));
                return;
            }

            spdlog::info("Texture exported to {}", path);
            popup->destroy();
        });
    }

    void TextureEditor::refreshTimeline()
    {
        if (!m_canvas_handle.is_alive() || !m_timeline_handle.is_alive())
            return;

        core::ecs::entities::UIDrawingCanvas canvas(core::ecs::EntityRegistry::GetEntityFromId(m_canvas_handle));
        core::ecs::entities::UIGrid timeline(core::ecs::EntityRegistry::GetEntityFromId(m_timeline_handle));
        const auto &image = canvas.getComponentMutable<core::components::UIDrawingCanvas>().image;

        std::vector<std::string> frames;
        for (std::size_t i = 0; i < image.frameCount(); i++)
            frames.push_back(std::to_string(i + 1));

        m_visibility_label_handles.clear();
        timeline.setLabels(image.layerNames(), frames);
        timeline.select(image.currentLayer(), image.currentFrameIndex(), false);

        refreshAllLayerVisible();
    }

    void TextureEditor::stepFrame(int step)
    {
        if (!m_canvas_handle.is_alive() || !m_timeline_handle.is_alive())
            return;

        core::ecs::entities::UIDrawingCanvas canvas(core::ecs::EntityRegistry::GetEntityFromId(m_canvas_handle));
        auto &canvas_comp = canvas.getComponentMutable<core::components::UIDrawingCanvas>();
        const int count = static_cast<int>(canvas_comp.image.frameCount());
        if (count <= 0)
            return;

        const int frame = ((canvas_comp.image.currentFrameIndex() + step) % count + count) % count;
        canvas_comp.image.selectFrame(static_cast<std::uint8_t>(frame));
        canvas_comp.texture_dirty = true;

        core::ecs::entities::UIGrid timeline(core::ecs::EntityRegistry::GetEntityFromId(m_timeline_handle));
        timeline.select(canvas_comp.image.currentLayer(), frame, false);
    }

    void TextureEditor::setPlaying(bool playing)
    {
        m_playing = playing;
        if (!m_canvas_handle.is_alive())
            return;

        core::ecs::entities::UIDrawingCanvas canvas(core::ecs::EntityRegistry::GetEntityFromId(m_canvas_handle));
        canvas.getComponentMutable<core::components::UIDrawingCanvas>().drawing_locked = playing;
    }

    void TextureEditor::tickAnimation(float deltaTime)
    {
        if (!m_playing)
            return;

        const float frameDuration = 1.0f / static_cast<float>(m_fps);
        m_frame_time_accumulator += deltaTime;

        int steps = 0;
        while (m_frame_time_accumulator >= frameDuration) {
            m_frame_time_accumulator -= frameDuration;
            steps++;
        }
        if (steps > 0)
            stepFrame(steps);
    }

    void TextureEditor::refreshAllLayerVisible()
    {
        if (!m_canvas_handle.is_alive())
            return;

        core::ecs::entities::UIDrawingCanvas canvas(core::ecs::EntityRegistry::GetEntityFromId(m_canvas_handle));
        const auto &image = canvas.getComponentMutable<core::components::UIDrawingCanvas>().image;
        const auto names = image.layerNames();

        auto isVisible = [&](const std::string &name) { return image.isLayerVisible(name); };
        if (m_allLayerVisible && std::none_of(names.begin(), names.end(), isVisible))
            m_allLayerVisible = false;
        else if (!m_allLayerVisible && std::all_of(names.begin(), names.end(), isVisible))
            m_allLayerVisible = true;

        if (!m_toggle_all_label_handle.is_alive())
            return;

        core::ecs::entities::UILabel label(core::ecs::EntityRegistry::GetEntityFromId(m_toggle_all_label_handle));
        label.setText(m_allLayerVisible ? "Hide" : "Show");
    }
} // namespace atmo::editor

ATMO_REGISTER_EDITOR(atmo::editor::TextureEditor);
