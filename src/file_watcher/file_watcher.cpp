#include <memory>
#include <mutex>
#include "core/event/events/file_system_event/file_system_event.hpp"
#ifdef __linux__


#include "file_watcher.hpp"
#include "core/event/event_registry.hpp"
#include "core/thread/thread_pool.hpp"
#include "spdlog/spdlog.h"
#include "core/event/events/file_system_event/file_event/file_create_event.hpp"
#include "core/event/events/file_system_event/file_event/file_delete_event.hpp"
#include "core/event/events/file_system_event/file_event/file_move_event.hpp"
#include "core/event/events/file_system_event/folder_event/folder_create_event.hpp"
#include "core/event/events/file_system_event/folder_event/folder_delete_event.hpp"
#include "core/event/events/file_system_event/folder_event/folder_move_event.hpp"

#include "editor/editor_entities/ui_file_explorer/ui_dir_node/ui_dir_node.hpp"
#include "editor/editor_entities/ui_file_explorer/ui_file_node/ui_file_node.hpp"

#include <cerrno>
#include <cstring>
#include <sys/inotify.h>
#include <sys/select.h>
#include <unistd.h>
#include <filesystem>
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

    void FileWatcher::AddWatch(const std::string &path)
    {
        auto &inst = Instance();

        if (inst.m_inotify_fd < 0)
            return;

        std::lock_guard lock(inst.m_watchMutex);

        if (inst.m_path_to_wd.count(path))
            return;

        int wd = inotify_add_watch(inst.m_inotify_fd, path.c_str(), kWatchFlags);
        if (wd < 0) {
            spdlog::warn("FileWatcher: inotify_add_watch failed on '{}' ({})", path, strerror(errno));
            return;
        }

        inst.m_wd_to_path[wd] = path;
        inst.m_path_to_wd[path] = wd;
    }

    void FileWatcher::RemoveWatch(const std::string &path)
    {
        auto &inst = Instance();

        if (inst.m_inotify_fd < 0)
            return;

        std::lock_guard lock(inst.m_watchMutex);

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
                    spdlog::warn("file name: {}", event->name);
                    changes.file_context = {.wd=event->wd, .mask=event->mask, .cookie=event->cookie, .file_name=event->name};/*    , .len=event->len, .name=event};   */
                    changes.wd_path = watched_path;

                    if (!watched_path.empty()) {
                        if (event->mask & IN_CREATE) {
                            if (event->mask & IN_ISDIR) {
                                auto folderCreate = atmo::core::event::EventRegistry::Create<atmo::core::event::events::FolderCreateEvent>("Event::FileSystemEvent::FolderCreateEvent");
                                folderCreate->change = changes;
                                atmo::core::event::EventRegistry::Dispatch(folderCreate);
                            } else {
                                auto fileCreate = atmo::core::event::EventRegistry::Create<atmo::core::event::events::FileCreateEvent>("Event::FileSystemEvent::FileCreateEvent");
                                fileCreate->change = changes;
                                atmo::core::event::EventRegistry::Dispatch(fileCreate);
                            }

                        } else if (event->mask & (IN_DELETE | IN_DELETE_SELF)) {
                            if (event->mask & IN_ISDIR) {
                                auto folderDelete = atmo::core::event::EventRegistry::Create<atmo::core::event::events::FolderDeleteEvent>("Event::FileSystemEvent::FolderDeleteEvent");
                                folderDelete->change = changes;
                                atmo::core::event::EventRegistry::Dispatch(folderDelete);
                            } else {
                                auto fileDelete = atmo::core::event::EventRegistry::Create<atmo::core::event::events::FileDeleteEvent>("Event::FileSystemEvent::FileDeleteEvent");
                                fileDelete->change = changes;
                                atmo::core::event::EventRegistry::Dispatch(fileDelete);
                            }

                        } else if (event->mask & (IN_MOVED_FROM | IN_MOVED_TO | IN_MOVE_SELF)) {
                            if (event->mask & IN_ISDIR) {
                                auto folderMove = atmo::core::event::EventRegistry::Create<atmo::core::event::events::FolderMoveEvent>("Event::FileSystemEvent::FolderMoveEvent");
                                folderMove->change = changes;
                                atmo::core::event::EventRegistry::Dispatch(folderMove);
                            } else {
                                auto fileMove = atmo::core::event::EventRegistry::Create<atmo::core::event::events::FileMoveEvent>("Event::FileSystemEvent::FileMoveEvent");
                                fileMove->change = changes;
                                atmo::core::event::EventRegistry::Dispatch(fileMove);
                            }

                        } else
                            spdlog::info("modified");
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

    FileWatcher::FileSystemNode FileWatcher::ScanFolder(const std::string &path, bool recursive) {
        FileWatcher::FileSystemNode node;

        if (!std::filesystem::exists(path)) {
            spdlog::warn("Path does not exist: {} (nothing openned)", path);
            return node;
        }
        if (!std::filesystem::is_directory(path)) {
            spdlog::warn("Not a directory: {} (nothing openned)", path);
            return node;
        }

        node.path = path;
        node.is_dir = true;

        for (const auto &entry : std::filesystem::directory_iterator(path)) {
            FileWatcher::FileSystemNode child;
            if (entry.is_directory()) {
                child = FileWatcher::ScanFolder(entry.path());
            } else {
                child.child.clear();
                child.path = entry.path();
                child.is_dir = false;
            }
            node.child.push_back(child);
        }
        return node;
    }

    void FileWatcher::InitFileSystem(const std::string &root) {
        FileWatcher::FileSystemNode rootNode;

        FileWatcher::Instance().m_root = ScanFolder(root);
    }

    std::shared_ptr<atmo::core::ecs::entities::UIFileExplorerFileNode> FileWatcher::createFoldableTreeLeaf(const FileSystemNode &file,
                                                                                     bool show_hidden) {

        auto node = core::ecs::EntityRegistry::Create<atmo::core::ecs::entities::UIFileExplorerFileNode>("Entity::UI::UIRect::UIButton::UIFileExplorerFileNode");

        auto &comp = node->getComponentMutable<atmo::core::components::UIFileExplorerNode>();
        node->setPath(file.path);

        comp.full_path = file.path;
        comp.is_directory = file.is_dir;
        comp.open = false;

        return node;
    }

    std::shared_ptr<atmo::core::ecs::entities::UIFileExplorerDirNode> FileWatcher::createFoldableTree(const FileSystemNode &root,
                                                                                     const std::vector<std::string> &open_paths,
                                                                                     bool show_hidden) {

        auto node = core::ecs::EntityRegistry::Create<atmo::core::ecs::entities::UIFileExplorerDirNode>("Entity::UI::UIRect::UIFoldableTreeItem::UIFileExplorerDirNode");

        auto &comp = node->getComponentMutable<atmo::core::components::UIFileExplorerNode>();
        node->setPath(root.path, show_hidden);

        bool should_be_open = std::find(open_paths.begin(), open_paths.end(), root.path) != open_paths.end();
        if (!should_be_open) {
            comp.open = false;
            return node;
        }
        comp.open = true;

        for (auto file : root.child) {

            if (file.is_dir) {
                auto newChild = createFoldableTree(file, open_paths, show_hidden);

                newChild->setParent(node->getChildContainer());
            } else {
                auto newChild = createFoldableTreeLeaf(file, show_hidden);

                newChild->setParent(node->getChildContainer());
            }
        }
        return node;
    }


    std::shared_ptr<atmo::core::ecs::entities::UIFileExplorerDirNode> FileWatcher::GetFileSystemFoldableTree(const std::vector<std::string> &open_paths, bool show_hidden) {
        std::lock_guard lock(Instance().m_graphMutex);

        return Instance().createFoldableTree(Instance().m_root, open_paths, show_hidden);
    }

} // namespace atmo

#endif // __linux__
