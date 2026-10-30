#include "ui_file_explorer.hpp"
#include "clay.h"
#include "core/ecs/entities/entity.hpp"
#include "core/ecs/entities/ui/ui.hpp"
#include "core/ecs/entities/ui/ui_button/ui_button.hpp"
#include "core/ecs/entities/ui/ui_label/ui_label.hpp"
#include "core/ecs/entities/ui/ui_layout.hpp"
#include "core/ecs/entities/ui/ui_rect/ui_rect.hpp"
#include "core/ecs/entity_registry.hpp"
#include "core/event/event_registry.hpp"
#include "core/event/events/file_system_event/file_selected_event.hpp"
#include "core/event/events/file_system_event/reload_explorer_event.hpp"
#include "core/types.hpp"
#include "editor/editor_entities/ui_popup/ui_popup.hpp"
#include "file_watcher/file_watcher.hpp"
#include "meta/auto_register.hpp"
#include "spdlog/spdlog.h"

#include <filesystem>
#include <fstream>
#include <functional>
#include <vector>

namespace fs = std::filesystem;

namespace atmo::core::ecs::entities
{
    void UIFileExplorer::RegisterSystems(flecs::world *world) {}

    void UIFileExplorer::initialize()
    {
        UIRect::initialize();
        setComponent<components::UIFileExplorer>({});

        createSignal<>("FileChange");
        createSignal<std::string>("FileFocus");

        auto &lay = getComponentMutable<core::components::Layout>();
        lay.direction = core::components::Layout::Direction::Vertical;
        lay.child_gap = 4;
        lay.padding = { 8, 8, 8, 8 };
        lay.width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        lay.height.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;

        auto toolbar = core::ecs::EntityRegistry::Create("Entity::UI::UIRect");
        auto &toolbar_layout = toolbar->getComponentMutable<core::components::Layout>();
        toolbar_layout.direction = core::components::Layout::Direction::Vertical;
        toolbar_layout.child_gap = 6;
        toolbar_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        toolbar_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::FIT;
        toolbar->setParent(*this);

        auto add_container = core::ecs::EntityRegistry::Create<UIRect>("Entity::UI::UIRect");
        auto &add_container_layout = add_container->getComponentMutable<core::components::Layout>();
        add_container_layout.direction = core::components::Layout::Direction::Horizontal;
        add_container_layout.child_alignment.horizontal = core::components::Layout::ChildAlignment::Start;
        add_container_layout.child_alignment.vertical = core::components::Layout::ChildAlignment::Start;
        add_container_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        add_container_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::FIXED;
        add_container_layout.height.size = core::components::Layout::SizingAxis::MinMax{ 28.0f, 28.0f };
        add_container->setParent(*toolbar);

        auto add_btn = core::ecs::EntityRegistry::Create<UIButton>("Entity::UI::UIRect::UIButton");
        auto add_btn_label = core::ecs::EntityRegistry::Create<UILabel>("Entity::UI::UILabel");
        add_btn_label->setText("Add File");
        add_btn_label->setFontSize(12);
        add_btn_label->getComponentMutable<core::components::UI>().modulate = core::types::Color::BLACK;
        add_btn_label->setParent(*add_btn);
        add_btn->setParent(*add_container);

        auto add_input = core::ecs::EntityRegistry::Create<UITextInput>("Entity::UI::UIInput::UITextInput");
        add_input->setPlaceholder("File Name");
        add_input->setParent(*add_container);

        auto add_folder_container = core::ecs::EntityRegistry::Create<UIRect>("Entity::UI::UIRect");
        auto &add_folder_container_layout = add_folder_container->getComponentMutable<core::components::Layout>();
        add_folder_container_layout.direction = core::components::Layout::Direction::Horizontal;
        add_folder_container_layout.child_alignment.horizontal = core::components::Layout::ChildAlignment::Start;
        add_folder_container_layout.child_alignment.vertical = core::components::Layout::ChildAlignment::Start;
        add_folder_container_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        add_folder_container_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::FIXED;
        add_folder_container_layout.height.size = core::components::Layout::SizingAxis::MinMax{ 28.0f, 28.0f };
        add_folder_container->setParent(*toolbar);

        auto add_folder_btn = core::ecs::EntityRegistry::Create<UIButton>("Entity::UI::UIRect::UIButton");
        auto add_folder_btn_label = core::ecs::EntityRegistry::Create<UILabel>("Entity::UI::UILabel");
        add_folder_btn_label->setText("Add Folder");
        add_folder_btn_label->setFontSize(12);
        add_folder_btn_label->getComponentMutable<core::components::UI>().modulate = core::types::Color::BLACK;
        add_folder_btn_label->setParent(*add_folder_btn);
        add_folder_btn->setParent(*add_folder_container);

        auto add_folder_input = core::ecs::EntityRegistry::Create<UITextInput>("Entity::UI::UIInput::UITextInput");
        add_folder_input->setValue("Folder Name");
        add_folder_input->setParent(*add_folder_container);

        auto rename_container = core::ecs::EntityRegistry::Create<UIRect>("Entity::UI::UIRect");
        auto &rename_container_layout = rename_container->getComponentMutable<core::components::Layout>();
        rename_container_layout.direction = core::components::Layout::Direction::Horizontal;
        rename_container_layout.child_alignment.vertical = core::components::Layout::ChildAlignment::Start;
        rename_container_layout.child_alignment.horizontal = core::components::Layout::ChildAlignment::Start;
        rename_container_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        rename_container_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::FIXED;
        rename_container_layout.height.size = core::components::Layout::SizingAxis::MinMax{ 28.0f, 28.0f };
        rename_container->setParent(*toolbar);

        auto rename_btn = core::ecs::EntityRegistry::Create<UIButton>("Entity::UI::UIRect::UIButton");
        auto rename_btn_label = core::ecs::EntityRegistry::Create<UILabel>("Entity::UI::UILabel");
        rename_btn_label->setText("Rename");
        rename_btn_label->setFontSize(12);
        rename_btn_label->getComponentMutable<core::components::UI>().modulate = core::types::Color::BLACK;
        rename_btn_label->setParent(*rename_btn);
        rename_btn->setParent(*rename_container);

        auto rename_input = core::ecs::EntityRegistry::Create<UITextInput>("Entity::UI::UIInput::UITextInput");
        rename_input->setPlaceholder("File Name");
        rename_input->setParent(*rename_container);

        auto delete_btn = core::ecs::EntityRegistry::Create<UIButton>("Entity::UI::UIRect::UIButton");
        auto delete_btn_label = core::ecs::EntityRegistry::Create<UILabel>("Entity::UI::UILabel");
        delete_btn_label->setText("Delete");
        delete_btn_label->setFontSize(12);
        delete_btn_label->getComponentMutable<core::components::UI>().modulate = core::types::Color::BLACK;
        delete_btn_label->setParent(*delete_btn);
        delete_btn->setParent(*toolbar);

        auto tree_container = core::ecs::EntityRegistry::Create<core::ecs::entities::UIRect>("Entity::UI::UIRect");
        auto &tree_layout = tree_container->getComponentMutable<core::components::Layout>();
        tree_layout.direction = core::components::Layout::Direction::Vertical;
        tree_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        tree_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        tree_layout.child_gap = 2;
        tree_layout.padding.left = 2;
        tree_layout.clip.horizontal = true;
        tree_layout.clip.vertical = true;
        auto &tree_rect = tree_container->getComponentMutable<core::components::UIRect>();
        tree_rect.color = core::types::Color("#ffffff");
        tree_container->setParent(*this);

        auto &explorer_comp = getComponentMutable<components::UIFileExplorer>();
        explorer_comp.add_input = add_input->getHandle();
        explorer_comp.add_folder_input = add_folder_input->getHandle();
        explorer_comp.rename_input = rename_input->getHandle();
        explorer_comp.tree_container = tree_container->getHandle();

        auto handle = p_handle;

        add_btn->getSignal<>("Pressed").connect([handle]() {
            if (!handle.is_alive()) {
                return;
            }
            UIFileExplorer explorer(core::ecs::EntityRegistry::GetEntityFromId(handle));


            auto &comp = explorer.getComponentMutable<components::UIFileExplorer>();
            if (comp.focused_path.empty())
                return;

            UITextInput input = explorer.getAddInput();
            std::string new_name = input.getText();
            if (new_name.empty())
                return;

            bool isDir = std::filesystem::is_directory(comp.focused_path);

            std::string target_dir = isDir ? comp.focused_path.string() : comp.focused_path.parent_path().string();

            fs::path new_path = fs::path(target_dir) / new_name;

            if (fs::exists(new_path)) {
                spdlog::warn("UIFileExplorer: '{}' already exists, cancel", new_path.string());
                return;
            }

            std::ofstream(new_path).close();
        });

        add_folder_btn->getSignal<>("Pressed").connect([handle]() {
            if (!handle.is_alive())
                return;

            UIFileExplorer explorer(core::ecs::EntityRegistry::GetEntityFromId(handle));
            auto &comp = explorer.getComponentMutable<components::UIFileExplorer>();
            if (comp.focused_path.empty())
                return;

            UITextInput input = explorer.getAddFolderInput();
            std::string new_name = input.getComponentMutable<core::components::UITextInput>().value;
            if (new_name.empty())
                return;

            bool isDir = std::filesystem::is_directory(comp.focused_path);

            std::string target_dir = isDir ? comp.focused_path.string() : comp.focused_path.parent_path().string();

            fs::path new_path = fs::path(target_dir) / new_name;

            if (fs::exists(new_path)) {
                spdlog::warn("UIFileExplorer: '{}' already exists, cancel", new_path.string());
                return;
            }

            std::error_code ec;
            fs::create_directory(new_path, ec);
            if (ec) {
                spdlog::warn("UIFileExplorer: folder creation failed '{}': {}", new_path.string(), ec.message());
                return;
            }
        });

        delete_btn->getSignal<>("Pressed").connect([handle]() {
            if (!handle.is_alive()) {
                return;
            }
            UIFileExplorer explorer(core::ecs::EntityRegistry::GetEntityFromId(handle));
            auto &comp = explorer.getComponentMutable<core::components::UIFileExplorer>();
            if (comp.focused_path.empty())
                return;

            auto delete_popup = core::ecs::EntityRegistry::Create<core::ecs::entities::UIPopup>("Entity::UI::UIRect::UIPopup");
            delete_popup->setParent(explorer);
            auto delete_bg = core::ecs::EntityRegistry::Create<core::ecs::entities::UIRect>("Entity::UI::UIRect");
            delete_bg->getComponentMutable<core::components::Layout>().width.type = core::components::Layout::SizingAxis::SizingAxisType::PERCENT;
            delete_bg->getComponentMutable<core::components::Layout>().width.size = 0.35f;
            delete_bg->getComponentMutable<core::components::Layout>().height.type = core::components::Layout::SizingAxis::SizingAxisType::PERCENT;
            delete_bg->getComponentMutable<core::components::Layout>().height.size = 0.25f;
            delete_bg->getComponentMutable<core::components::Layout>().direction = core::components::Layout::Direction::Vertical;
            delete_bg->getComponentMutable<core::components::Layout>().child_alignment.horizontal = core::components::Layout::ChildAlignment::Center;
            delete_bg->getComponentMutable<core::components::Layout>().padding = { 8, 8, 8, 8 };
            delete_bg->getComponentMutable<core::components::Layout>().child_gap = 8;
            delete_bg->getComponentMutable<core::components::UIRect>().color = core::types::Color("#9f9f9f");
            delete_bg->setParent(*delete_popup);

            auto delete_editor_top_bar = core::ecs::EntityRegistry::Create<core::ecs::entities::UI>("Entity::UI");
            delete_editor_top_bar->getComponentMutable<core::components::Layout>().direction = core::components::Layout::Direction::Horizontal;
            delete_editor_top_bar->getComponentMutable<core::components::Layout>().width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
            delete_editor_top_bar->getComponentMutable<core::components::Layout>().height.type = core::components::Layout::SizingAxis::SizingAxisType::FIXED;
            delete_editor_top_bar->getComponentMutable<core::components::Layout>().height.size = core::components::Layout::SizingAxis::MinMax{ 32.0f, 32.0f };
            delete_editor_top_bar->setParent(*delete_bg);

            auto label = core::ecs::EntityRegistry::Create<core::ecs::entities::UILabel>("Entity::UI::UILabel");
            label->setFontPath("project://assets/fonts/Nunito/Nunito.ttf");
            label->setText(std::format("Delete {}", comp.focused_path.string()));
            label->setFontBold(false);
            label->setFontSize(12);
            label->getComponentMutable<core::components::UI>().modulate = core::types::Color::BLACK;
            label->setParent(*delete_editor_top_bar);

            auto close_btn_holder = core::ecs::EntityRegistry::Create<core::ecs::entities::UI>("Entity::UI");
            close_btn_holder->getComponentMutable<core::components::Layout>().width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
            close_btn_holder->getComponentMutable<core::components::Layout>().height.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
            close_btn_holder->getComponentMutable<core::components::Layout>().child_alignment.horizontal = core::components::Layout::ChildAlignment::End;
            close_btn_holder->setParent(*delete_editor_top_bar);
            auto close_open_editor_btn = core::ecs::EntityRegistry::Create<core::ecs::entities::UIButton>("Entity::UI::UIRect::UIButton");
            auto &close_open_editor_btn_rect = close_open_editor_btn->getComponentMutable<core::components::UIRect>();
            close_open_editor_btn_rect.color = core::types::Color::RED;
            auto &close_open_editor_btn_layout = close_open_editor_btn->getComponentMutable<core::components::Layout>();
            close_open_editor_btn_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
            close_open_editor_btn_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
            close_open_editor_btn_layout.aspect_ratio = { 1.0f, 1.0f };
            close_open_editor_btn->setParent(*close_btn_holder);
            close_open_editor_btn->getSignal<>("Released").connect([delete_popup]() { delete_popup->destroy(); });

            auto confirm_btn = core::ecs::EntityRegistry::Create<UIButton>("Entity::UI::UIRect::UIButton");
            auto confirm_btn_label = core::ecs::EntityRegistry::Create<UILabel>("Entity::UI::UILabel");
            confirm_btn_label->setText("Confirm");
            confirm_btn_label->setFontSize(12);
            confirm_btn_label->getComponentMutable<core::components::UI>().modulate = core::types::Color::BLACK;
            confirm_btn_label->setParent(*confirm_btn);
            confirm_btn->setParent(*delete_bg);
            confirm_btn->getSignal<>("Released").connect([handle, delete_popup]() {
                if (!handle.is_alive()) {
                    return;
                }
                UIFileExplorer explorer(core::ecs::EntityRegistry::GetEntityFromId(handle));
                auto &comp = explorer.getComponentMutable<core::components::UIFileExplorer>();
                bool isDir = std::filesystem::is_directory(comp.focused_path);

                std::error_code ec;
                if (isDir) {
                    fs::remove_all(comp.focused_path, ec);
                } else {
                    fs::remove(comp.focused_path, ec);
                }

                if (ec) {
                    spdlog::warn("UIFileExplorer: deletion failed '{}': {}", comp.focused_path.string(), ec.message());
                    return;
                }

                delete_popup->destroy();
            });
        });


        rename_btn->getSignal<>("Pressed").connect([handle]() {
            if (!handle.is_alive()) {
                return;
            }
            UIFileExplorer explorer(core::ecs::EntityRegistry::GetEntityFromId(handle));

            auto &comp = explorer.getComponentMutable<components::UIFileExplorer>();
            if (comp.focused_path.empty())
                return;

            UITextInput input = explorer.getRenameInput();
            std::string new_name = input.getText();
            if (new_name.empty())
                return;

            fs::path old_path(comp.focused_path);
            fs::path new_path = old_path.parent_path() / new_name;

            std::error_code ec;
            fs::rename(old_path, new_path, ec);
            if (ec) {
                spdlog::warn("UIFileExplorer: renamme failed '{}' -> '{}': {}", old_path.string(), new_path.string(), ec.message());
                return;
            }

            comp.focused_path = new_path.string();
        });


        createSignal<>("Reload");
        getSignal<>("Reload").connect([handle]() {
            if (!handle.is_alive()) {
                return;
            }
            UIFileExplorer explorer(core::ecs::EntityRegistry::GetEntityFromId(handle));
            explorer.rebuild();
        });
        atmo::core::event::EventRegistry::SetCallBack<atmo::core::event::events::ReloadExplorerEvent>([handle](event::events::ReloadExplorerEvent *evt) {
            if (!handle.is_alive()) {
                return;
            }
            UIFileExplorer explorer(core::ecs::EntityRegistry::GetEntityFromId(handle));
            explorer.getSignal<>("Reload").emit();
        });

        atmo::core::event::EventRegistry::SetCallBack<atmo::core::event::events::FileSelectedEvent>([handle](event::events::FileSelectedEvent *evt) {
            if (!handle.is_alive()) {
                return;
            }
            UIFileExplorer explorer(core::ecs::EntityRegistry::GetEntityFromId(handle));
            auto &comp = explorer.getComponentMutable<atmo::core::components::UIFileExplorer>();
            comp.focused_path = evt->path;
        });

        rebuild();
    }

    bool UIFileExplorer::hasFocus() const
    {
        return false;
        // return (getComponent<components::UIFileExplorer>().focused_node != flecs::entity{});
    }

    void UIFileExplorer::rebuild()
    {
        auto &comp = getComponentMutable<components::UIFileExplorer>();
        UIRect tree_container = getTreeContainer();

        std::vector<std::string> open_paths;
        std::filesystem::path focused_node = comp.focused_path;
        std::function<void(UIFileExplorerDirNode)> collect = [&](UIFileExplorerDirNode dir) {
            if (!dir.isOpen())
                return;

            open_paths.push_back(dir.path());

            UIRect container = dir.getChildContainer();
            for (auto &child : container.getChildren()) {
                if (!child.hasComponent<components::UIFileExplorerNode>())
                    continue;

                auto &child_node = child.getComponentMutable<components::UIFileExplorerNode>();
                if (child_node.is_directory)
                    collect(UIFileExplorerDirNode(child));
            }
        };

        for (auto &child : tree_container.getChildren()) {
            if (child.hasComponent<components::UIFileExplorerNode>())
                collect(UIFileExplorerDirNode(child));
        }

        for (auto &child : tree_container.getChildren()) child.destroy();

        const FileWatcher::FileSystemNode &tree = FileWatcher::GetTree();
        if (tree.path.empty())
            return;

        auto root_node = CreateDirNode(tree, open_paths, focused_node);
        root_node->setParent(tree_container);
    }

    std::shared_ptr<UIFileExplorerFileNode> UIFileExplorer::CreateFileNode(const FileWatcher::FileSystemNode &file, const std::filesystem::path &focused_path)
    {
        auto node = core::ecs::EntityRegistry::Create<UIFileExplorerFileNode>("Entity::UI::UIRect::UIButton::UIFileExplorerFileNode");

        auto &btnComp = node->getComponentMutable<core::components::UIButton>();
        btnComp.toggle = true;
        btnComp.group = -1;
        auto nodeHandle = node->getHandle();

        node->getSignal<bool>("Toggle").connect([nodeHandle](bool state) {
            if (!nodeHandle.is_alive()) {
                return;
            }

            UIFileExplorerFileNode node(nodeHandle);
            auto &uiComp = node.getComponentMutable<core::components::UIRect>();
            if (state) {
                node.getSignal<>("Pressed").emit();
                uiComp.color = core::types::Color::BLACK;
                uiComp.color.a = 0.2f;
            } else {
                uiComp.color.a = 0.0f;
            }
        });

        node->setPath(file.path);

        if (file.path == focused_path) {
            node->getSignal<bool>("Toggle").emit(true);
        }

        return node;
    }

    std::shared_ptr<UIFileExplorerDirNode>
    UIFileExplorer::CreateDirNode(const FileWatcher::FileSystemNode &dir, const std::vector<std::string> &open_paths, const std::filesystem::path &focused_path)
    {
        auto node = core::ecs::EntityRegistry::Create<UIFileExplorerDirNode>("Entity::UI::UIRect::UIFoldableTreeItem::UIFileExplorerDirNode");
        node->setPath(dir.path);

        auto btn = node->getTitleButton();
        auto &btnComp = btn.getComponentMutable<core::components::UIButton>();
        btnComp.toggle = true;
        btnComp.group = -1;
        auto btnHandle = btn.getHandle();
        btn.getSignal<bool>("Toggle").connect([btnHandle](bool state) {
            if (!btnHandle.is_alive()) {
                return;
            }

            UIButton node(btnHandle);
            auto &uiComp = node.getComponentMutable<core::components::UIRect>();
            if (state) {
                node.getSignal<>("Pressed").emit();
                uiComp.color = core::types::Color::BLACK;
                uiComp.color.a = 0.2f;
            } else {
                uiComp.color.a = 0.0f;
            }
        });

        if (dir.path == focused_path) {
            btn.getSignal<bool>("Toggle").emit(true);
        }
        bool should_be_open = std::find(open_paths.begin(), open_paths.end(), dir.path.string()) != open_paths.end();
        if (should_be_open) {
            auto &treeComp = node->getComponentMutable<core::components::UIFoldableTreeItem>();
            treeComp.open = true;
        }

        for (const auto &child : dir.child) {
            if (child.is_dir)
                CreateDirNode(child, open_paths, focused_path)->setParent(node->getChildContainer());
            else
                CreateFileNode(child, focused_path)->setParent(node->getChildContainer());
        }
        return node;
    }

    Clay_ElementDeclaration UIFileExplorer::buildDecl()
    {
        auto d = UIRect::buildDecl();
        return d;
    }

    void UIFileExplorer::draw() {}

    UITextInput UIFileExplorer::getAddInput() const
    {
        return UITextInput(getComponent<components::UIFileExplorer>().add_input);
    }

    UITextInput UIFileExplorer::getAddFolderInput() const
    {
        return UITextInput(getComponent<components::UIFileExplorer>().add_folder_input);
    }

    UITextInput UIFileExplorer::getRenameInput() const
    {
        return UITextInput(getComponent<components::UIFileExplorer>().rename_input);
    }

    UIRect UIFileExplorer::getTreeContainer() const
    {
        return UIRect(getComponent<components::UIFileExplorer>().tree_container);
    }
} // namespace atmo::core::ecs::entities

ATMO_REGISTER_ENTITY(entities::UIFileExplorer)
ATMO_REGISTER_COMPONENT(atmo::core::components::UIFileExplorer)
