#include "ui_file_explorer.hpp"
#include "clay.h"
#include "core/ecs/entities/entity.hpp"
#include "core/ecs/entities/ui/ui.hpp"
#include "core/ecs/entities/ui/ui_layout.hpp"
#include "core/ecs/entity_registry.hpp"
#include "editor/editor_entities/ui_popup/ui_popup.hpp"
#include "meta/auto_register.hpp"
#include "spdlog/spdlog.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <functional>

namespace fs = std::filesystem;

namespace atmo::core::ecs::entities
{
    namespace
    {
        constexpr std::string_view ToolbarName = "FileExplorer toolbar";
        constexpr std::string_view AddButtonContainer = "Add Button Container";
        constexpr std::string_view AddButtonName = "FileExplorer add button";
        constexpr std::string_view AddInputName = "FileExplorer add input";
        constexpr std::string_view DeleteButtonName = "FileExplorer delete button";
        constexpr std::string_view RenameButtonContainer = "Rename Button Container";
        constexpr std::string_view RenameButtonName = "FileExplorer rename button";
        constexpr std::string_view RenameInputName = "FileExplorer rename input";
        constexpr std::string_view RefreshButtonName = "FileExplorer refresh button";
        constexpr std::string_view TreeContainerName = "FileExplorer tree container";
    } // namespace

    void UIFileExplorer::RegisterSystems(flecs::world *) {}

    void UIFileExplorer::initialize()
    {
        UIRect::initialize();
        setComponent<components::UIFileExplorer>({});

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
        toolbar->rename(std::string(ToolbarName));
        toolbar->setParent(*this);

        auto add_container = core::ecs::EntityRegistry::Create<UIButton>("Entity::UI::UIRect");
        auto &add_container_layout = add_container->getComponentMutable<core::components::Layout>();
        add_container_layout.direction = core::components::Layout::Direction::Horizontal;
        add_container_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        add_container_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::FIXED;
        add_container_layout.height.size = core::components::Layout::SizingAxis::MinMax{ 28.0f, 28.0f };
        add_container->rename(std::string(AddButtonContainer));
        add_container->setParent(*toolbar);

        auto add_btn = core::ecs::EntityRegistry::Create<UIButton>("Entity::UI::UIRect::UIButton");
        add_btn->rename(std::string(AddButtonName));
        add_btn->setParent(*add_container);
        UILabel(add_btn->getChild("Button label")).setText("Add");

        auto add_input = core::ecs::EntityRegistry::Create<UITextInput>("Entity::UI::UIInput::UITextInput");
        add_input->rename(std::string(AddInputName));
        add_input->setParent(*add_container);

        auto rename_container = core::ecs::EntityRegistry::Create<UIButton>("Entity::UI::UIRect");
        auto &rename_container_layout = rename_container->getComponentMutable<core::components::Layout>();
        rename_container_layout.direction = core::components::Layout::Direction::Horizontal;
        rename_container_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        rename_container_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::FIXED;
        rename_container_layout.height.size = core::components::Layout::SizingAxis::MinMax{ 28.0f, 28.0f };
        rename_container->rename(std::string(RenameButtonContainer));
        rename_container->setParent(*toolbar);

        auto rename_btn = core::ecs::EntityRegistry::Create<UIButton>("Entity::UI::UIRect::UIButton");
        rename_btn->rename(std::string(RenameButtonName));
        rename_btn->setParent(*rename_container);
        UILabel(rename_btn->getChild("Button label")).setText("Rename");

        auto rename_input = core::ecs::EntityRegistry::Create<UITextInput>("Entity::UI::UIInput::UITextInput");
        rename_input->rename(std::string(RenameInputName));
        rename_input->setParent(*rename_container);

        auto delete_btn = core::ecs::EntityRegistry::Create<UIButton>("Entity::UI::UIRect::UIButton");
        delete_btn->rename(std::string(DeleteButtonName));
        delete_btn->setParent(*toolbar);
        UILabel(delete_btn->getChild("Button label")).setText("Delete");

        auto refresh_btn = core::ecs::EntityRegistry::Create<UIButton>("Entity::UI::UIRect::UIButton");
        refresh_btn->rename(std::string(RefreshButtonName));
        refresh_btn->setParent(*toolbar);
        UILabel(refresh_btn->getChild("Button label")).setText("Refresh");

        auto tree_container = core::ecs::EntityRegistry::Create("Entity::UI::UIRect");
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
        tree_container->rename(std::string(TreeContainerName));
        tree_container->setParent(*this);

        auto handle = p_handle;

        add_btn->getSignal<>("Pressed").connect([handle]() {
            if (!handle.is_alive()) {
                return;
            }
            UIFileExplorer explorer(core::ecs::EntityRegistry::GetEntityFromId(handle));


            auto &comp = explorer.getComponentMutable<components::UIFileExplorer>();
            if (comp.focused_node == flecs::entity{})
                return;

            UITextInput input = explorer.getAddInput();
            std::string new_name = input.getComponentMutable<core::components::UITextInput>().value;
            if (new_name.empty())
                return;

            std::string target_dir = comp.focused_is_directory ? comp.focused_path : fs::path(comp.focused_path).parent_path().string();

            fs::path new_path = fs::path(target_dir) / new_name;

            if (fs::exists(new_path)) {
                spdlog::warn("UIFileExplorer: '{}' existe déjà, création annulée", new_path.string());
                return;
            }

            std::ofstream(new_path).close();

            explorer.rebuild();
        });


        delete_btn->getSignal<>("Pressed").connect([handle]() {
            if (!handle.is_alive()) {
                return;
            }
            UIFileExplorer explorer(core::ecs::EntityRegistry::GetEntityFromId(handle));
            auto &comp = explorer.getComponentMutable<core::components::UIFileExplorer>();
            if (comp.focused_node == flecs::entity{}) {
                return;
            }

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
            label->setText(std::format("Delete {}", comp.focused_path));
            label->setFontBold(false);
            label->setFontSize(24);
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
            close_open_editor_btn->getChildren()[0].destroy();
            close_open_editor_btn->setParent(*close_btn_holder);
            close_open_editor_btn->getSignal<>("Released").connect([delete_popup]() { delete_popup->destroy(); });

            auto confirm_btn = core::ecs::EntityRegistry::Create<UIButton>("Entity::UI::UIRect::UIButton");
            confirm_btn->setParent(*delete_bg);
            UILabel(confirm_btn->getChild("Button label")).setText("Confirm");
            confirm_btn->getSignal<>("Released").connect([handle, delete_popup]() {
                if (!handle.is_alive()) {
                    return;
                }
                UIFileExplorer explorer(core::ecs::EntityRegistry::GetEntityFromId(handle));
                auto &comp = explorer.getComponentMutable<core::components::UIFileExplorer>();

                std::error_code ec;
                if (comp.focused_is_directory) {
                    fs::remove_all(comp.focused_path, ec);
                } else {
                    fs::remove(comp.focused_path, ec);
                }

                if (ec) {
                    spdlog::warn("UIFileExplorer: échec de suppression de '{}': {}", comp.focused_path, ec.message());
                    return;
                }

                explorer.rebuild();

                delete_popup->destroy();
            });
        });


        rename_btn->getSignal<>("Pressed").connect([handle]() {
            if (!handle.is_alive()) {
                return;
            }
            UIFileExplorer explorer(core::ecs::EntityRegistry::GetEntityFromId(handle));

            auto &comp = explorer.getComponentMutable<components::UIFileExplorer>();
            if (comp.focused_node == flecs::entity{})
                return;

            UITextInput input = explorer.getRenameInput();
            std::string new_name = input.getComponentMutable<core::components::UITextInput>().value;
            if (new_name.empty())
                return;

            fs::path old_path(comp.focused_path);
            fs::path new_path = old_path.parent_path() / new_name;

            std::error_code ec;
            fs::rename(old_path, new_path, ec);
            if (ec) {
                spdlog::warn("UIFileExplorer: échec du renommage '{}' -> '{}': {}", old_path.string(), new_path.string(), ec.message());
                return;
            }

            comp.focused_path = new_path.string();

            explorer.rebuild();
        });


        refresh_btn->getSignal<>("Pressed").connect([handle]() {
            if (!handle.is_alive()) {
                return;
            }
            UIFileExplorer explorer(core::ecs::EntityRegistry::GetEntityFromId(handle));
            explorer.rebuild();
        });
    }

    void UIFileExplorer::setRootPath(const std::string &path)
    {
        auto &comp = getComponentMutable<components::UIFileExplorer>();

        std::error_code ec;
        if (!fs::is_directory(path, ec) || ec) {
            spdlog::warn("UIFileExplorer: '{}' n'est pas un dossier valide", path);
            return;
        }

        comp.root_path = fs::canonical(path, ec).string();
        comp.focused_node = {};
        comp.focused_path.clear();

        rebuild();
    }

    void UIFileExplorer::setFocus(flecs::entity node, const std::string &path, bool is_directory)
    {
        auto &comp = getComponentMutable<components::UIFileExplorer>();

        if (comp.focused_node != flecs::entity{} && comp.focused_node.is_alive()) {
            auto &old_node = comp.focused_node.get_mut<components::UIFileExplorerNode>();
            if (old_node.is_directory) {
                UIFileExplorerDirNode old_dir(comp.focused_node);
                old_dir.setHighlight(false);
            } else {
                UIFileExplorerFileNode old_file(comp.focused_node);
                old_file.setHighlight(false);
            }
        }

        if (node != flecs::entity{} && node.is_alive()) {
            auto &new_node = node.get_mut<components::UIFileExplorerNode>();
            if (new_node.is_directory) {
                UIFileExplorerDirNode new_dir(node);
                new_dir.setHighlight(true);
            } else {
                UIFileExplorerFileNode new_file(node);
                new_file.setHighlight(true);
            }
        }

        comp.focused_node = node;
        comp.focused_path = path;
        comp.focused_is_directory = is_directory;

        getSignal<std::string>("FileFocus").emit(path);
    }

    bool UIFileExplorer::hasFocus() const
    {
        return (getComponent<components::UIFileExplorer>().focused_node != flecs::entity{});
    }

    void UIFileExplorer::rebuild()
    {
        auto &comp = getComponentMutable<components::UIFileExplorer>();
        UIRect tree_container = getTreeContainer();

        std::vector<std::string> open_paths;
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

        std::string old_focused_path = comp.focused_path;
        bool old_focus_was_directory = comp.focused_is_directory;

        for (auto &child : tree_container.getChildren()) child.destroy();

        auto root_node = core::ecs::EntityRegistry::Create<UIFileExplorerDirNode>("Entity::UI::UIRect::UIFoldableTreeItem::UIFileExplorerDirNode");
        root_node->setParent(tree_container);
        root_node->getComponentMutable<components::UIFileExplorerNode>().explorer_root = p_handle;

        root_node->setPath(comp.root_path, comp.show_hidden, true);

        reopenPaths(*root_node, open_paths, comp.show_hidden);

        comp.focused_node = {};
        comp.focused_path.clear();

        if (!old_focused_path.empty()) {
            flecs::entity found = findNodeByPath(*root_node, old_focused_path);
            if (found.is_valid() && found.is_alive()) {
                comp.focused_node = found;
                comp.focused_path = old_focused_path;
                comp.focused_is_directory = old_focus_was_directory;
            }
        }
    }

    void UIFileExplorer::reopenPaths(UIFileExplorerDirNode &node, const std::vector<std::string> &open_paths, bool show_hidden)
    {
        const std::string &my_path = node.path();

        bool should_be_open = std::find(open_paths.begin(), open_paths.end(), my_path) != open_paths.end();
        if (!should_be_open)
            return;

        auto &node_comp = node.getComponentMutable<components::UIFileExplorerNode>();
        if (!node_comp.scanned) {
            node.openAndScan(show_hidden);
        }

        UIRect container = node.getChildContainer();
        for (auto &child : container.getChildren()) {
            if (!child.hasComponent<components::UIFileExplorerNode>())
                continue;

            auto &child_node = child.getComponentMutable<components::UIFileExplorerNode>();
            if (child_node.is_directory) {
                UIFileExplorerDirNode child_dir(child);
                reopenPaths(child_dir, open_paths, show_hidden);
            }
        }
    }

    flecs::entity UIFileExplorer::findNodeByPath(UIFileExplorerDirNode &node, const std::string &target_path)
    {
        if (node.path() == target_path)
            return node.getHandle();

        UIRect container = node.getChildContainer();
        for (auto &child : container.getChildren()) {
            if (!child.hasComponent<components::UIFileExplorerNode>())
                continue;

            auto &child_node = child.getComponentMutable<components::UIFileExplorerNode>();
            if (child_node.full_path == target_path)
                return child;

            if (child_node.is_directory) {
                UIFileExplorerDirNode child_dir(child);
                flecs::entity found = findNodeByPath(child_dir, target_path);
                if (found.is_valid())
                    return found;
            }
        }

        return flecs::entity::null();
    }

    UIButton UIFileExplorer::getAddButton() const
    {
        return UIButton(getChild(ToolbarName).getChild(AddButtonName));
    }
    UIButton UIFileExplorer::getDeleteButton() const
    {
        return UIButton(getChild(ToolbarName).getChild(DeleteButtonName));
    }
    UIButton UIFileExplorer::getRenameButton() const
    {
        return UIButton(getChild(ToolbarName).getChild(RenameButtonName));
    }
    UIButton UIFileExplorer::getRefreshButton() const
    {
        return UIButton(getChild(ToolbarName).getChild(RefreshButtonName));
    }
    UITextInput UIFileExplorer::getAddInput() const
    {
        return UITextInput(getChild(ToolbarName).getChild(AddButtonContainer).getChild(AddInputName));
    }
    UITextInput UIFileExplorer::getRenameInput() const
    {
        return UITextInput(getChild(ToolbarName).getChild(RenameButtonContainer).getChild(RenameInputName));
    }
    UIRect UIFileExplorer::getTreeContainer() const
    {
        return UIRect(getChild(TreeContainerName));
    }

    Clay_ElementDeclaration UIFileExplorer::buildDecl()
    {
        return UIRect::buildDecl();
    }

    void UIFileExplorer::draw(ClaySdL3RendererData *) {}
} // namespace atmo::core::ecs::entities

ATMO_REGISTER_ENTITY(entities::UIFileExplorer)
ATMO_REGISTER_COMPONENT(atmo::core::components::UIFileExplorer)
