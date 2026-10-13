#if defined(_WIN32)

#include "core/thread/thread_pool.hpp"
#include "file_watcher_backend.hpp"
#include "spdlog/spdlog.h"

#include <array>
#include <cstddef>
#include <optional>
#include <string_view>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace fs = std::filesystem;

namespace atmo::file_watcher
{
    namespace
    {
        constexpr DWORD NotifyFilter = FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME;
        constexpr std::size_t NotifyBufferSize = 64 * 1024;
        constexpr std::chrono::milliseconds PollInterval{ 50 };

        /**
         * @brief One overlapped ReadDirectoryChangesW on the root watches the whole tree. Its completion is polled from the thread pool.
         */
        class Win32Backend final : public Backend
        {
        public:
            Win32Backend(const fs::path &root, DirChangedCallback on_dir_changed) : m_root(root), m_on_dir_changed(std::move(on_dir_changed))
            {
                m_dir = CreateFileW(
                    m_root.c_str(),
                    FILE_LIST_DIRECTORY,
                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                    nullptr,
                    OPEN_EXISTING,
                    FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
                    nullptr);
                if (m_dir == INVALID_HANDLE_VALUE) {
                    DWORD error = GetLastError();
                    spdlog::error("FileWatcher: cannot open '{}' (error {})", m_root.string(), error);
                    return;
                }

                m_overlapped.hEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
                if (!m_overlapped.hEvent) {
                    spdlog::error("FileWatcher: CreateEventW failed (error {})", GetLastError());
                    return;
                }

                if (!issueRead())
                    return;

                m_task = core::ThreadPool::Instance().repeat([this]() { poll(); }, -1, PollInterval);
            }

            ~Win32Backend() override
            {
                if (m_task) {
                    m_task->cancel();
                    m_task->wait();
                }

                // The kernel writes into m_buffer until the pending read completes: cancel it and wait before freeing anything.
                if (m_read_pending) {
                    CancelIoEx(m_dir, &m_overlapped);
                    DWORD bytes = 0;
                    GetOverlappedResult(m_dir, &m_overlapped, &bytes, TRUE);
                }

                if (m_overlapped.hEvent)
                    CloseHandle(m_overlapped.hEvent);
                if (m_dir != INVALID_HANDLE_VALUE)
                    CloseHandle(m_dir);
            }

            bool isValid() const noexcept
            {
                return m_task.has_value();
            }

        private:
            bool issueRead()
            {
                m_read_pending =
                    ReadDirectoryChangesW(m_dir, m_buffer.data(), static_cast<DWORD>(m_buffer.size()), TRUE, NotifyFilter, nullptr, &m_overlapped, nullptr);
                if (!m_read_pending) {
                    DWORD error = GetLastError();
                    spdlog::error("FileWatcher: ReadDirectoryChangesW failed on '{}' (error {})", m_root.string(), error);
                }
                return m_read_pending;
            }

            void poll()
            {
                if (!m_read_pending)
                    return;

                DWORD bytes = 0;
                if (!GetOverlappedResult(m_dir, &m_overlapped, &bytes, FALSE)) {
                    DWORD error = GetLastError();
                    if (error == ERROR_IO_INCOMPLETE)
                        return;

                    m_read_pending = false;
                    if (error == ERROR_NOTIFY_ENUM_DIR) {
                        m_on_dir_changed(m_root, true);
                        issueRead();
                    } else {
                        spdlog::error("FileWatcher: watch on '{}' stopped (error {})", m_root.string(), error);
                    }
                    return;
                }

                m_read_pending = false;

                // Zero bytes means the buffer overflowed and the changes were lost.
                if (bytes == 0) {
                    m_on_dir_changed(m_root, true);
                } else {
                    const std::byte *ptr = m_buffer.data();
                    while (true) {
                        const auto *info = reinterpret_cast<const FILE_NOTIFY_INFORMATION *>(ptr);
                        std::wstring_view name(info->FileName, info->FileNameLength / sizeof(WCHAR));

                        // FileName is the changed item relative to the root: its parent lists it.
                        m_on_dir_changed((m_root / name).parent_path(), false);

                        if (info->NextEntryOffset == 0)
                            break;
                        ptr += info->NextEntryOffset;
                    }
                }

                issueRead();
            }

            fs::path m_root;
            DirChangedCallback m_on_dir_changed;

            HANDLE m_dir = INVALID_HANDLE_VALUE;
            OVERLAPPED m_overlapped{};
            alignas(DWORD) std::array<std::byte, NotifyBufferSize> m_buffer{};
            bool m_read_pending = false;
            std::optional<core::TaskHandle> m_task;
        };
    } // namespace

    std::unique_ptr<Backend> CreateBackend(const fs::path &root, Backend::DirChangedCallback on_dir_changed)
    {
        auto backend = std::make_unique<Win32Backend>(root, std::move(on_dir_changed));
        if (!backend->isValid())
            return nullptr;
        return backend;
    }
} // namespace atmo::file_watcher

#endif // _WIN32
