#if defined(__linux__)

#include "core/thread/thread_pool.hpp"
#include "file_watcher_backend.hpp"
#include "spdlog/spdlog.h"

#include <algorithm>
#include <cerrno>
#include <climits>
#include <cstring>
#include <optional>
#include <sys/inotify.h>
#include <unistd.h>
#include <unordered_map>

namespace fs = std::filesystem;

namespace atmo::file_watcher
{
    namespace
    {
        constexpr uint32_t WatchFlags = IN_CREATE | IN_DELETE | IN_MOVED_FROM | IN_MOVED_TO | IN_ONLYDIR;
        constexpr std::size_t EventBufferSize = (sizeof(inotify_event) + NAME_MAX + 1) * 32;
        constexpr std::chrono::milliseconds PollInterval{ 50 };

        bool IsUnder(const fs::path &path, const fs::path &dir)
        {
            auto [dir_end, path_it] = std::mismatch(dir.begin(), dir.end(), path.begin(), path.end());
            return dir_end == dir.end();
        }

        /**
         * @brief inotify watches are not recursive: one watch per directory, added and removed as directories appear, move and disappear.
         */
        class InotifyBackend final : public Backend
        {
        public:
            InotifyBackend(const fs::path &root, DirChangedCallback on_dir_changed) : m_root(root), m_on_dir_changed(std::move(on_dir_changed))
            {
                m_fd = inotify_init1(IN_NONBLOCK | IN_CLOEXEC);
                if (m_fd < 0) {
                    spdlog::error("FileWatcher: inotify_init1 failed ({})", strerror(errno));
                    return;
                }

                addWatchRecursive(m_root);
                m_task = core::ThreadPool::Instance().repeat([this]() { poll(); }, -1, PollInterval);
            }

            ~InotifyBackend() override
            {
                if (m_task) {
                    m_task->cancel();
                    m_task->wait();
                }

                // Closing the descriptor removes every watch.
                if (m_fd >= 0)
                    close(m_fd);
            }

            bool isValid() const noexcept
            {
                return m_fd >= 0;
            }

        private:
            void addWatch(const fs::path &dir)
            {
                int wd = inotify_add_watch(m_fd, dir.c_str(), WatchFlags);
                if (wd < 0) {
                    spdlog::warn("FileWatcher: inotify_add_watch failed on '{}' ({})", dir.string(), strerror(errno));
                    return;
                }
                m_wd_to_path[wd] = dir;
            }

            void addWatchRecursive(const fs::path &dir)
            {
                addWatch(dir);

                std::error_code ec;
                fs::recursive_directory_iterator it(dir, fs::directory_options::skip_permission_denied, ec);
                for (; !ec && it != fs::recursive_directory_iterator(); it.increment(ec)) {
                    std::error_code entry_ec;
                    if (it->is_directory(entry_ec) && !it->is_symlink(entry_ec))
                        addWatch(it->path());
                }
            }

            void removeWatchRecursive(const fs::path &dir)
            {
                for (auto it = m_wd_to_path.begin(); it != m_wd_to_path.end();) {
                    if (IsUnder(it->second, dir)) {
                        inotify_rm_watch(m_fd, it->first);
                        it = m_wd_to_path.erase(it);
                    } else {
                        ++it;
                    }
                }
            }

            void handleEvent(const inotify_event &event)
            {
                if (event.mask & IN_Q_OVERFLOW) {
                    m_on_dir_changed(m_root, true);
                    return;
                }

                auto it = m_wd_to_path.find(event.wd);
                if (it == m_wd_to_path.end())
                    return;

                if (event.mask & IN_IGNORED) {
                    m_wd_to_path.erase(it);
                    return;
                }

                if (event.len == 0)
                    return;

                const fs::path dir = it->second;
                const fs::path item = dir / event.name;

                // A moved directory keeps its watches under the old path: drop them and watch it again where it lands.
                if (event.mask & IN_ISDIR) {
                    if (event.mask & IN_MOVED_FROM)
                        removeWatchRecursive(item);
                    else if (event.mask & (IN_CREATE | IN_MOVED_TO))
                        addWatchRecursive(item);
                }

                m_on_dir_changed(dir, false);
            }

            void poll()
            {
                alignas(inotify_event) char buf[EventBufferSize];

                while (true) {
                    ssize_t len = read(m_fd, buf, sizeof(buf));
                    if (len <= 0) {
                        if (len < 0 && errno != EAGAIN && errno != EWOULDBLOCK)
                            spdlog::error("FileWatcher: read() failed ({})", strerror(errno));
                        return;
                    }

                    for (const char *ptr = buf; ptr < buf + len;) {
                        const auto *event = reinterpret_cast<const inotify_event *>(ptr);
                        handleEvent(*event);
                        ptr += sizeof(inotify_event) + event->len;
                    }
                }
            }

            fs::path m_root;
            DirChangedCallback m_on_dir_changed;

            int m_fd = -1;
            std::unordered_map<int, fs::path> m_wd_to_path;
            std::optional<core::TaskHandle> m_task;
        };
    } // namespace

    std::unique_ptr<Backend> CreateBackend(const fs::path &root, Backend::DirChangedCallback on_dir_changed)
    {
        auto backend = std::make_unique<InotifyBackend>(root, std::move(on_dir_changed));
        if (!backend->isValid())
            return nullptr;
        return backend;
    }
} // namespace atmo::file_watcher

#endif // __linux__
