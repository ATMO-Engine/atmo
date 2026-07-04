#pragma once

#if !defined(ATMO_EXPORT)

#include <filesystem>
#include <string>
#include <vector>

#if defined(_WIN32)
#include <windows.h>
#else
#include <sys/types.h>
#endif

namespace atmo::editor
{
    /**
     * @brief Minimal cross-platform spawn/poll/terminate utility for a single OS child process.
     *        Inherits the parent's stdout/stderr/stdin (no redirection), so the child's logs are
     *        interleaved with the editor's in the same terminal.
     */
    class ChildProcess
    {
    public:
        ChildProcess() = default;
        ~ChildProcess();

        ChildProcess(const ChildProcess &) = delete;
        ChildProcess &operator=(const ChildProcess &) = delete;

        ChildProcess(ChildProcess &&other) noexcept;
        ChildProcess &operator=(ChildProcess &&other) noexcept;

        /**
         * @brief Spawns @p executable as a child process.
         *
         * @param executable Absolute path to the binary to run.
         * @param args Arguments, NOT including argv[0] (the executable path is used as argv[0]).
         * @return true if the process was successfully spawned.
         */
        bool spawn(const std::filesystem::path &executable, const std::vector<std::string> &args);

        /**
         * @brief Whether the process is still alive. On POSIX, reaps the process (waitpid WNOHANG)
         *        as a side effect once it has exited.
         */
        bool isRunning();

        /**
         * @brief Terminates the process if it is running (SIGTERM on POSIX, TerminateProcess on
         *        Windows). No-op if not running.
         */
        void terminate();

    private:
        void reset();

#if defined(_WIN32)
        PROCESS_INFORMATION m_process_info{};
        bool m_has_process = false;
#else
        pid_t m_pid = -1;
#endif
    };
} // namespace atmo::editor

#endif
