#include "child_process.hpp"

#if !defined(ATMO_EXPORT)

#if defined(_WIN32)
#include <sstream>
#else
#include <csignal>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>
extern char **environ;
#endif

namespace atmo::editor
{
    ChildProcess::~ChildProcess()
    {
#if defined(_WIN32)
        if (m_has_process) {
            CloseHandle(m_process_info.hProcess);
            CloseHandle(m_process_info.hThread);
        }
#endif
    }

    ChildProcess::ChildProcess(ChildProcess &&other) noexcept
    {
#if defined(_WIN32)
        m_process_info = other.m_process_info;
        m_has_process = other.m_has_process;
#else
        m_pid = other.m_pid;
#endif
        other.reset();
    }

    ChildProcess &ChildProcess::operator=(ChildProcess &&other) noexcept
    {
        if (this != &other) {
            this->~ChildProcess();
#if defined(_WIN32)
            m_process_info = other.m_process_info;
            m_has_process = other.m_has_process;
#else
            m_pid = other.m_pid;
#endif
            other.reset();
        }
        return *this;
    }

    void ChildProcess::reset()
    {
#if defined(_WIN32)
        m_process_info = PROCESS_INFORMATION{};
        m_has_process = false;
#else
        m_pid = -1;
#endif
    }

#if defined(_WIN32)
    static std::string BuildCommandLine(const std::filesystem::path &executable, const std::vector<std::string> &args)
    {
        auto quote = [](const std::string &s) {
            std::string out = "\"";
            for (char c : s) {
                if (c == '"')
                    out += '\\';
                out += c;
            }
            out += '"';
            return out;
        };

        std::ostringstream oss;
        oss << quote(executable.string());
        for (const auto &arg : args) oss << ' ' << quote(arg);
        return oss.str();
    }

    bool ChildProcess::spawn(const std::filesystem::path &executable, const std::vector<std::string> &args)
    {
        std::string cmdline = BuildCommandLine(executable, args);
        std::vector<char> mutable_cmdline(cmdline.begin(), cmdline.end());
        mutable_cmdline.push_back('\0');

        STARTUPINFOA si{};
        si.cb = sizeof(si);
        PROCESS_INFORMATION pi{};

        BOOL ok = CreateProcessA(executable.string().c_str(), mutable_cmdline.data(), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi);

        if (!ok)
            return false;

        m_process_info = pi;
        m_has_process = true;
        return true;
    }

    bool ChildProcess::isRunning()
    {
        if (!m_has_process)
            return false;

        DWORD code = 0;
        if (!GetExitCodeProcess(m_process_info.hProcess, &code))
            return false;

        if (code != STILL_ACTIVE) {
            CloseHandle(m_process_info.hProcess);
            CloseHandle(m_process_info.hThread);
            reset();
            return false;
        }

        return true;
    }

    void ChildProcess::terminate()
    {
        if (!m_has_process)
            return;

        TerminateProcess(m_process_info.hProcess, 1);
        CloseHandle(m_process_info.hProcess);
        CloseHandle(m_process_info.hThread);
        reset();
    }
#else
    bool ChildProcess::spawn(const std::filesystem::path &executable, const std::vector<std::string> &args)
    {
        std::string exe_str = executable.string();

        std::vector<char *> argv;
        argv.push_back(exe_str.data());
        std::vector<std::string> arg_storage = args;
        for (auto &arg : arg_storage) argv.push_back(arg.data());
        argv.push_back(nullptr);

        pid_t pid = -1;
        int result = posix_spawn(&pid, exe_str.c_str(), nullptr, nullptr, argv.data(), environ);

        if (result != 0)
            return false;

        m_pid = pid;
        return true;
    }

    bool ChildProcess::isRunning()
    {
        if (m_pid < 0)
            return false;

        int status = 0;
        pid_t result = waitpid(m_pid, &status, WNOHANG);

        if (result == 0)
            return true;

        m_pid = -1;
        return false;
    }

    void ChildProcess::terminate()
    {
        if (m_pid < 0)
            return;

        kill(m_pid, SIGTERM);

        int status = 0;
        waitpid(m_pid, &status, 0);
        m_pid = -1;
    }
#endif
} // namespace atmo::editor

#endif
