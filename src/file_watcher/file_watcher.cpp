#include <algorithm>
#include <memory>
#include <mutex>
#include <string>
#include "core/ecs/entities/ui/ui_button/ui_button.hpp"
#include "core/ecs/entities/ui/ui_foldable_tree_item/ui_foldable_tree_item.hpp"
#include "core/ecs/entities/ui/ui_rect/ui_rect.hpp"
#include "core/event/events/file_system_event/reload_explorer_event.hpp"
#include "core/types.hpp"
#include "project/project_manager.hpp"
#ifdef __linux__


#include "core/event/event_registry.hpp"
#include "core/thread/thread_pool.hpp"
#include "file_watcher.hpp"
#include "spdlog/spdlog.h"

#include "editor/editor_entities/ui_file_explorer/ui_dir_node/ui_dir_node.hpp"
#include "editor/editor_entities/ui_file_explorer/ui_file_node/ui_file_node.hpp"

#include <cerrno>
#include <cstring>
#include <filesystem>
#include <sys/inotify.h>
#include <sys/select.h>
#include <unistd.h>
#include <vector>

namespace atmo
{
    constexpr size_t kEventBufSize = (sizeof(inotify_event) + NAME_MAX + 1) * 32;
    constexpr uint32_t kWatchFlags = IN_CREATE | IN_DELETE | IN_MOVED_FROM | IN_MOVED_TO | IN_MODIFY | IN_ONLYDIR | IN_DELETE_SELF | IN_MOVE_SELF;

    FileWatcher &FileWatcher::Instance()
    {
        static FileWatcher inst;
        return inst;
    }

    FileWatcher::FileWatcher()
    {
        m_inotify_fd = inotify_init1(IN_NONBLOCK);
        if (m_inotify_fd < 0) {
            spdlog::error("FileWatcher: inotify_init1 failed ({})", strerror(errno));
            return;
        }

        auto taskHandle = core::ThreadPool::Instance().repeat([this]() { workerLoop(); }, -1, std::chrono::milliseconds{ 50 });
    }

    FileWatcher::~FileWatcher()
    {
        ClearWatches();

        if (m_inotify_fd >= 0) {
            close(m_inotify_fd);
            m_inotify_fd = -1;
        }
    }

    void FileWatcher::AddWatch(const std::filesystem::path &path)
    {
        auto &inst = Instance();

        if (inst.m_inotify_fd < 0)
            return;

        if (inst.m_path_to_wd.count(path))
            return;

        int wd = inotify_add_watch(inst.m_inotify_fd, path.c_str(), kWatchFlags);
        if (wd < 0) {
            spdlog::warn("FileWatcher: inotify_add_watch failed on '{}' ({})", path.string(), strerror(errno));
            return;
        }

        inst.m_wd_to_path[wd] = path;
        inst.m_path_to_wd[path] = wd;
    }

    void FileWatcher::RemoveWatch(const std::filesystem::path &path)
    {
        auto &inst = Instance();

        if (inst.m_inotify_fd < 0)
            return;

        auto it = inst.m_path_to_wd.find(path);
        if (it == inst.m_path_to_wd.end())
            return;

        int wd = it->second;
        inotify_rm_watch(inst.m_inotify_fd, wd);
        inst.m_wd_to_path.erase(wd);
        inst.m_path_to_wd.erase(it);
    }

    void FileWatcher::ClearWatches()
    {
        auto &inst = Instance();

        if (inst.m_inotify_fd < 0)
            return;

        std::lock_guard lock(inst.m_watchMutex);

        for (auto &[wd, path] : inst.m_wd_to_path) inotify_rm_watch(inst.m_inotify_fd, wd);

        inst.m_wd_to_path.clear();
        inst.m_path_to_wd.clear();
    }

    FileWatcher::FileSystemNode *FileWatcher::getWorkingNode(const FileSystemNode *root, const std::filesystem::path &changePath)
    {
        std::filesystem::path relativePath;
        try {
            relativePath = changePath.lexically_relative(atmo::project::ProjectManager::GetCurrentProjectPath());
        } catch (const std::filesystem::filesystem_error &e) {
            spdlog::error("Failed to get relative Path {} from root {}: {}", changePath.string(), m_root.path.string(), e.what());
            return nullptr;
        }

        FileSystemNode *current = &m_root;
        if (relativePath != ".") {
            spdlog::debug("Wd path: {}", changePath.string());
            spdlog::debug("Root path: {}", m_root.path.string());
            spdlog::debug("Relative path: {}", relativePath.string());
            for (const auto &part : relativePath) {
                const std::string name = part.string();
                auto it = std::find_if(
                    current->child.begin(), current->child.end(), [&name](const FileSystemNode &node) { return node.path.filename().string() == name; });
                if (it == current->child.end()) {
                    spdlog::warn("Could not find {} while navigating {}", name, changePath.string());
                    return nullptr;
                }
                current = &(*it);
            }
        }
        return current;
    }

    void FileWatcher::createAction(const inotify_event *evt, const FileChange &changes)
    {
        FileSystemNode *current = getWorkingNode(&m_root, changes.wd_path);

        if (evt->mask & IN_ISDIR) {
            std::string name(evt->name);
            auto path = std::filesystem::path(changes.wd_path / name);
            auto newNode = ScanFolder(path);
            spdlog::info("complete path {}, wd path: {},  file name: {}", path.string(), changes.wd_path.string(), name);
            spdlog::info("New folder {}", newNode.path.string());
            current->child.push_back(newNode);
        } else {
            std::string name(evt->name);
            FileWatcher::FileSystemNode newNode;
            newNode.path = std::filesystem::path(changes.wd_path / name);
            newNode.is_dir = false;
            newNode.wd = -1;

            spdlog::info("complete path {}, wd path: {},  file name: {}", newNode.path.string(), changes.wd_path.string(), name);
            current->child.push_back(newNode);
        }

        auto reload = atmo::core::event::EventRegistry::Create<atmo::core::event::events::ReloadExplorerEvent>("Event::ReloadExplorerEvent");
        atmo::core::event::EventRegistry::Dispatch(reload);
    }

    void FileWatcher::deleteAction(const inotify_event *evt, const FileChange &changes)
    {
        FileSystemNode *current = getWorkingNode(&m_root, changes.wd_path);

        if (evt->mask & IN_ISDIR) {
            std::string name(evt->name);
            auto path = std::filesystem::path(changes.wd_path / name);
            RemoveWatch(path);
            spdlog::info("complete path {}, wd path: {},  file name: {}", path.string(), changes.wd_path.string(), name);

            auto it = std::find_if(
                current->child.begin(), current->child.end(), [&name](const FileSystemNode &node) { return node.path.filename().string() == name; });

            if (it != current->child.end()) {
                current->child.erase(it);
            }
        } else {
            std::string name(evt->name);
            auto path = std::filesystem::path(changes.wd_path / name);
            spdlog::info("complete path {}, wd path: {},  file name: {}", path.string(), changes.wd_path.string(), name);

            auto it = std::find_if(
                current->child.begin(), current->child.end(), [&name](const FileSystemNode &node) { return node.path.filename().string() == name; });

            if (it != current->child.end()) {
                current->child.erase(it);
            }
        }

        auto reload = atmo::core::event::EventRegistry::Create<atmo::core::event::events::ReloadExplorerEvent>("Event::ReloadExplorerEvent");
        atmo::core::event::EventRegistry::Dispatch(reload);
    }

    void FileWatcher::moveInAction(const inotify_event *evt, const FileChange &changes)
    {
        createAction(evt, changes);
        return;
    }

    void FileWatcher::moveOutAction(const inotify_event *evt, const FileChange &changes)
    {
        deleteAction(evt, changes);
        return;
    }

    void FileWatcher::workerLoop()
    {
        if (m_inotify_fd < 0)
            return;

        char buf[kEventBufSize] __attribute__((aligned(__alignof__(inotify_event))));

        std::lock_guard lock(m_watchMutex);

        ssize_t len = read(m_inotify_fd, buf, sizeof(buf));
        while (len > 0) {
            const char *ptr = buf;

            while (ptr < buf + len) {
                const auto *event = reinterpret_cast<const inotify_event *>(ptr);

                if (event->len > 0) {
                    std::string watched_path;

                    {
                        auto it = m_wd_to_path.find(event->wd);
                        if (it != m_wd_to_path.end())
                            watched_path = it->second;
                    }

                    FileChange changes;
                    changes.file_context = {
                        .wd = event->wd, .mask = event->mask, .cookie = event->cookie, .file_name = event->name
                    }; /*    , .len=event->len, .name=event};   */
                    changes.wd_path = watched_path;

                    if (!watched_path.empty()) {
                        if (event->mask & IN_CREATE) {
                            spdlog::debug("File system event triggered, file created");
                            createAction(event, changes);
                        } else if (event->mask & (IN_DELETE)) {
                            spdlog::debug("File system event triggered, file delete");
                            deleteAction(event, changes);
                        } else if (event->mask & IN_MOVED_FROM) {
                            spdlog::debug("File system event triggered, file moved from");
                            moveOutAction(event, changes);
                        } else if (event->mask & (IN_MOVED_TO)) {
                            spdlog::debug("File system event triggered, file moved to");
                            moveInAction(event, changes);
                        } else
                            spdlog::debug("File system event triggered, event not handled yet, event code :{}", event->mask);
                    }
                }

                ptr += sizeof(inotify_event) + event->len;
            }

            len = read(m_inotify_fd, buf, sizeof(buf));
        }

        if (len < 0) {
            if (!(errno == EAGAIN || errno == EWOULDBLOCK)) {
                spdlog::error("FileWatcher: read() failed ({})", strerror(errno));
            }
        }
    }

    FileWatcher::FileSystemNode FileWatcher::ScanFolder(const std::filesystem::path &path, bool recursive)
    {
        FileWatcher::FileSystemNode node;

        if (!std::filesystem::exists(path)) {
            spdlog::warn("Path does not exist: {} (nothing openned)", path.string());
            return node;
        }
        if (!std::filesystem::is_directory(path)) {
            spdlog::warn("Not a directory: {} (nothing openned)", path.string());
            return node;
        }

        node.path = path;
        node.is_dir = true;

        AddWatch(path);

        if (Instance().m_path_to_wd.find(path) == Instance().m_path_to_wd.end()) {
            spdlog::warn("The file {} was not added inside path to wd map, not scanning deeper", path.string());
            return node;
        }
        node.wd = Instance().m_path_to_wd[path];

        for (const auto &entry : std::filesystem::directory_iterator(path)) {
            FileWatcher::FileSystemNode child;
            if (entry.is_directory()) {
                child = FileWatcher::ScanFolder(entry.path());
            } else {
                child.child.clear();
                child.path = entry.path();
                child.is_dir = false;
                child.wd = -1;
            }
            node.child.push_back(child);
        }
        return node;
    }

    void FileWatcher::InitFileSystem(const std::filesystem::path &root)
    {
        FileWatcher::FileSystemNode rootNode;

        std::lock_guard lock(Instance().m_watchMutex);

        FileWatcher::Instance().m_root = ScanFolder(root);
    }

    std::shared_ptr<atmo::core::ecs::entities::UIFileExplorerFileNode>
    FileWatcher::createFoldableTreeLeaf(const FileSystemNode &file, const std::filesystem::path &focused_path)
    {
        auto node =
            core::ecs::EntityRegistry::Create<atmo::core::ecs::entities::UIFileExplorerFileNode>("Entity::UI::UIRect::UIButton::UIFileExplorerFileNode");

        auto &comp = node->getComponentMutable<atmo::core::components::UIFileExplorerNode>();

        auto &btnComp = node->getComponentMutable<core::components::UIButton>();
        btnComp.toggle = true;
        btnComp.group = -1;
        auto nodeHandle = node->getHandle();

        node->getSignal<bool>("Toggle").connect([nodeHandle](bool state) {
            if (!nodeHandle.is_alive()) {
                return;
            }

            atmo::core::ecs::entities::UIFileExplorerFileNode node(nodeHandle);
            auto &uiComp = node.getComponentMutable<atmo::core::components::UIRect>();
            if (state) {
                node.getSignal<>("Pressed").emit();
                uiComp.color = core::types::Color::BLACK;
                uiComp.color.a = 0.2f;
            } else {
                uiComp.color.a = 0.0f;
            }
        });

        node->setPath(file.path);
        comp.full_path = file.path;

        if (node->path() == focused_path) {
            node->getSignal<bool>("Toggle").emit(true);
        }

        return node;
    }

    std::shared_ptr<atmo::core::ecs::entities::UIFileExplorerDirNode>
    FileWatcher::createFoldableTree(const FileSystemNode &root, const std::vector<std::string> &open_paths, const std::filesystem::path &focused_path)
    {
        auto node = core::ecs::EntityRegistry::Create<atmo::core::ecs::entities::UIFileExplorerDirNode>(
            "Entity::UI::UIRect::UIFoldableTreeItem::UIFileExplorerDirNode");
        node->setPath(root.path);

        auto btn = node->getTitleButton();
        auto &btnComp = btn.getComponentMutable<core::components::UIButton>();
        btnComp.toggle = true;
        btnComp.group = -1;
        auto btnHandle = btn.getHandle();
        btn.getSignal<bool>("Toggle").connect([btnHandle](bool state) {
            if (!btnHandle.is_alive()) {
                return;
            }

            atmo::core::ecs::entities::UIButton node(btnHandle);
            auto &uiComp = node.getComponentMutable<atmo::core::components::UIRect>();
            if (state) {
                node.getSignal<>("Pressed").emit();
                uiComp.color = core::types::Color::BLACK;
                uiComp.color.a = 0.2f;
            } else {
                uiComp.color.a = 0.0f;
            }
        });

        if (node->path() == focused_path) {
            btn.getSignal<bool>("Toggle").emit(true);
        }
        bool should_be_open = std::find(open_paths.begin(), open_paths.end(), root.path) != open_paths.end();
        if (should_be_open) {
            auto &treeComp = node->getComponentMutable<atmo::core::components::UIFoldableTreeItem>();
            treeComp.open = true;
        }


        for (auto file : root.child) {

            if (file.is_dir) {
                auto newChild = createFoldableTree(file, open_paths, focused_path);

                newChild->setParent(node->getChildContainer());
            } else {
                auto newChild = createFoldableTreeLeaf(file, focused_path);

                newChild->setParent(node->getChildContainer());
            }
        }
        return node;
    }


    std::shared_ptr<atmo::core::ecs::entities::UIFileExplorerDirNode>
    FileWatcher::GetFileSystemFoldableTree(const std::vector<std::string> &open_paths, const std::filesystem::path &focused_path)
    {
        std::lock_guard lock(Instance().m_graphMutex);
        return Instance().createFoldableTree(Instance().m_root, open_paths, focused_path);
    }

} // namespace atmo

#endif // __linux__
