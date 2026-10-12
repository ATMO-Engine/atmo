#if defined(__APPLE__)

#include "file_watcher_backend.hpp"
#include "spdlog/spdlog.h"

#include <CoreServices/CoreServices.h>
#include <dispatch/dispatch.h>

namespace fs = std::filesystem;

namespace atmo::file_watcher
{
    namespace
    {
        constexpr CFTimeInterval Latency = 0.1;
        constexpr FSEventStreamCreateFlags StreamFlags =
            kFSEventStreamCreateFlagFileEvents | kFSEventStreamCreateFlagNoDefer | kFSEventStreamCreateFlagWatchRoot;
        constexpr FSEventStreamEventFlags LostEventsFlags =
            kFSEventStreamEventFlagMustScanSubDirs | kFSEventStreamEventFlagUserDropped | kFSEventStreamEventFlagKernelDropped;

        /**
         * @brief One FSEvents stream watches the whole tree. Callbacks run on a private serial dispatch queue.
         */
        class FSEventsBackend final : public Backend
        {
        public:
            FSEventsBackend(const fs::path &root, DirChangedCallback on_dir_changed) : m_root(root), m_on_dir_changed(std::move(on_dir_changed))
            {
                CFStringRef root_str = CFStringCreateWithFileSystemRepresentation(kCFAllocatorDefault, m_root.c_str());
                CFArrayRef paths = CFArrayCreate(kCFAllocatorDefault, reinterpret_cast<const void **>(&root_str), 1, &kCFTypeArrayCallBacks);

                FSEventStreamContext context{ .version = 0, .info = this, .retain = nullptr, .release = nullptr, .copyDescription = nullptr };
                m_stream =
                    FSEventStreamCreate(kCFAllocatorDefault, &FSEventsBackend::OnEvents, &context, paths, kFSEventStreamEventIdSinceNow, Latency, StreamFlags);

                CFRelease(paths);
                CFRelease(root_str);

                if (!m_stream) {
                    spdlog::error("FileWatcher: FSEventStreamCreate failed on '{}'", m_root.string());
                    return;
                }

                m_queue = dispatch_queue_create("atmo.file_watcher", DISPATCH_QUEUE_SERIAL);
                FSEventStreamSetDispatchQueue(m_stream, m_queue);

                m_started = FSEventStreamStart(m_stream);
                if (!m_started)
                    spdlog::error("FileWatcher: FSEventStreamStart failed on '{}'", m_root.string());
            }

            ~FSEventsBackend() override
            {
                if (m_stream) {
                    if (m_started)
                        FSEventStreamStop(m_stream);
                    FSEventStreamInvalidate(m_stream);
                    FSEventStreamRelease(m_stream);
                }

                if (m_queue) {
                    // Wait for a callback that may still be running on the queue before this object goes away.
                    dispatch_sync_f(m_queue, nullptr, [](void *) {});
                    dispatch_release(m_queue);
                }
            }

            bool isValid() const noexcept
            {
                return m_started;
            }

        private:
            static void
            OnEvents(ConstFSEventStreamRef, void *info, size_t count, void *event_paths, const FSEventStreamEventFlags flags[], const FSEventStreamEventId[])
            {
                auto *self = static_cast<FSEventsBackend *>(info);
                auto **paths = static_cast<char **>(event_paths);

                for (size_t i = 0; i < count; i++) {
                    if (flags[i] & kFSEventStreamEventFlagHistoryDone)
                        continue;

                    // The root itself was moved or deleted, or events were dropped: rescan everything.
                    if (flags[i] & kFSEventStreamEventFlagRootChanged || flags[i] & LostEventsFlags) {
                        self->m_on_dir_changed(self->m_root, true);
                        continue;
                    }

                    // With kFSEventStreamCreateFlagFileEvents the path is the item that changed: its parent lists it.
                    self->m_on_dir_changed(fs::path(paths[i]).parent_path(), false);
                }
            }

            fs::path m_root;
            DirChangedCallback m_on_dir_changed;

            FSEventStreamRef m_stream = nullptr;
            dispatch_queue_t m_queue = nullptr;
            bool m_started = false;
        };
    } // namespace

    std::unique_ptr<Backend> CreateBackend(const fs::path &root, Backend::DirChangedCallback on_dir_changed)
    {
        auto backend = std::make_unique<FSEventsBackend>(root, std::move(on_dir_changed));
        if (!backend->isValid())
            return nullptr;
        return backend;
    }
} // namespace atmo::file_watcher

#endif // __APPLE__
