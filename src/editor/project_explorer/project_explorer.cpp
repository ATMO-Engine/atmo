#include "project_explorer.hpp"
#include <filesystem>
#include "core/ecs/entities/ui/ui.hpp"
#include "core/ecs/entities/ui/ui_button/ui_button.hpp"
#include "core/ecs/entities/ui/ui_image/ui_image.hpp"
#include "core/ecs/entities/ui/ui_input/ui_text_input/ui_text_input.hpp"
#include "core/ecs/entities/ui/ui_label/ui_label.hpp"
#include "core/ecs/entities/ui/ui_layout.hpp"
#include "core/ecs/entities/ui/ui_rect/ui_rect.hpp"
#include "core/ecs/entity_registry.hpp"
#include "project/file_system.hpp"
#include "project/project_manager.hpp"
#include "spdlog/spdlog.h"

#if !defined(ATMO_EXPORT)
namespace atmo::editor
{
    ProjectExplorer::ProjectExplorer(atmo::core::Engine &engine) : m_engine(engine) {}

    void ProjectExplorer::init()
    {
        const auto found_projects = project::FileSystem::SearchFiles("user://projects/*/.atmo/project_settings.json");

        auto scene = m_engine.getECS().getCurrentScene();

        auto window_ui_container = core::ecs::EntityRegistry::Create<core::ecs::entities::UI>("Entity::UI");
        auto &window_ui_container_layout = window_ui_container->getComponentMutable<core::components::Layout>();
        window_ui_container_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        window_ui_container_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        window_ui_container_layout.direction = core::components::Layout::Direction::Vertical;
        window_ui_container_layout.child_alignment.horizontal = core::components::Layout::ChildAlignment::Center;
        window_ui_container->setParent(*scene);

        auto topbar_container = core::ecs::EntityRegistry::Create<core::ecs::entities::UI>("Entity::UI");
        auto &topbar_container_layout = topbar_container->getComponentMutable<core::components::Layout>();
        topbar_container_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        topbar_container_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::FIXED;
        topbar_container_layout.height.size = core::components::Layout::SizingAxis::MinMax{ 60.0f, 60.0f };
        topbar_container_layout.direction = core::components::Layout::Direction::Horizontal;
        topbar_container_layout.padding = { 16, 16, 16, 16 };
        topbar_container->setParent(*window_ui_container);

        auto topbar = core::ecs::EntityRegistry::Create<core::ecs::entities::UIRect>("Entity::UI::UIRect");
        auto &topbar_rect = topbar->getComponentMutable<core::components::UIRect>();
        topbar_rect.color = core::types::Color("#bbbbbb");
        topbar_rect.corner_radius = { 4, 4, 4, 4 };
        auto &topbar_layout = topbar->getComponentMutable<core::components::Layout>();
        topbar_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        topbar_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        topbar_layout.padding = { 8, 8, 8, 8 };
        topbar_layout.child_gap = 8;
        topbar->setParent(*topbar_container);

        auto create_btn = core::ecs::EntityRegistry::Create<core::ecs::entities::UIButton>("Entity::UI::UIRect::UIButton");
        core::ecs::entities::UILabel(create_btn->getChildren()[0]).setText("atmo.create_new_project");
        create_btn->getComponentMutable<core::components::UIRect>().corner_radius = { 4, 4, 4, 4 };
        create_btn->setParent(*topbar);

        auto create_btn_icon = core::ecs::EntityRegistry::Create<core::ecs::entities::UIImage>("Entity::UI::UIImage");
        create_btn_icon->setTexturePath("project://assets/icons/plus.svg");
        create_btn_icon->getComponentMutable<core::components::UI>().modulate = core::types::Color::BLACK;
        create_btn_icon->getComponentMutable<core::components::Layout>().width.type = core::components::Layout::SizingAxis::SizingAxisType::FIXED;
        create_btn_icon->getComponentMutable<core::components::Layout>().width.size = core::components::Layout::SizingAxis::MinMax{ 24.0f, 24.0f };
        create_btn_icon->getComponentMutable<core::components::Layout>().height.type = core::components::Layout::SizingAxis::SizingAxisType::FIXED;
        create_btn_icon->getComponentMutable<core::components::Layout>().height.size = core::components::Layout::SizingAxis::MinMax{ 24.0f, 24.0f };
        create_btn_icon->setParent(*create_btn);
        create_btn_icon->swap(create_btn->getChildren()[0]);

        auto new_project_name_input = core::ecs::EntityRegistry::Create<core::ecs::entities::UITextInput>("Entity::UI::UIInput::UITextInput");
        new_project_name_input->setValue("New Project Name");
        new_project_name_input->setParent(*topbar);

        create_btn->getSignal<>("Released").connect([&engine = m_engine, window_ui_container, input_handle = new_project_name_input->getHandle()]() {
            auto input = core::ecs::entities::UITextInput(input_handle);
            std::string project_name = input.getComponent<core::components::UITextInput>().value;

            try {
                project::ProjectManager::CreateProject("user://projects/" + project_name);
            } catch (std::exception &e) {
                spdlog::error("Couldn't create project with this name: {}", e.what());
                return;
            }

            core::SignalQueue::Enqueue([&engine, window_ui_container, project_name]() {
                window_ui_container->destroy();
                engine.launchEditor("user://projects/" + project_name);
            });
        });

        auto list_container = core::ecs::EntityRegistry::Create<core::ecs::entities::UI>("Entity::UI");
        auto &list_container_layout = list_container->getComponentMutable<core::components::Layout>();
        list_container_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        list_container_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        list_container_layout.padding = { 16, 16, 16, 16 };
        list_container->setParent(*window_ui_container);

        auto list = core::ecs::EntityRegistry::Create<core::ecs::entities::UIRect>("Entity::UI::UIRect");
        auto &list_rect = list->getComponentMutable<core::components::UIRect>();
        list_rect.color = core::types::Color("#bbbbbb");
        list_rect.corner_radius = { 4, 4, 4, 4 };
        auto &list_layout = list->getComponentMutable<core::components::Layout>();
        list_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        list_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        list_layout.padding = { 8, 8, 8, 8 };
        list_layout.child_gap = 8;
        list_layout.direction = core::components::Layout::Direction::Vertical;
        list_layout.clip.vertical = true;
        list->setParent(*list_container);


        spdlog::info("Found projects:");
        for (auto project : found_projects) {
            spdlog::info(" - {}", project);
            std::filesystem::path project_root = std::filesystem::path(project).parent_path().parent_path();
            makeProjectButton(*list, window_ui_container, project_root.string());
        }
    }

    void
    ProjectExplorer::makeProjectButton(core::ecs::entities::UI parent, std::shared_ptr<core::ecs::entities::UI> window_ui_container, std::string project_path)
    {
        auto btn = core::ecs::EntityRegistry::Create<core::ecs::entities::UIButton>("Entity::UI::UIRect::UIButton");

        auto btn_layout = btn->getComponentMutable<core::components::Layout>();
        btn_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        btn_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::FIXED;
        btn_layout.height.size = core::components::Layout::SizingAxis::MinMax{ 64.0f, 64.0f };
        btn->getComponentMutable<core::components::UIRect>().corner_radius = { 4, 4, 4, 4 };
        core::ecs::entities::UILabel(btn->getChildren()[0]).setText(project_path);

        btn->getSignal<>("Released").connect([&engine = m_engine, window_ui_container, project_path]() {
            core::SignalQueue::Enqueue([&engine, window_ui_container, project_path]() {
                window_ui_container->destroy();
                engine.launchEditor(project_path);
            });
        });

        btn->setParent(parent);
    }
} // namespace atmo::editor
#endif
