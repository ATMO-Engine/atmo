#include "impl/romver.hpp"

#include "romver.hpp"
#include "spdlog/spdlog.h"

#include <minwindef.h>
#include <string>

namespace atmo
{
    namespace impl
    {
        Romver::Romver()
        {
            m_project = 0;
            m_major = 0;
            m_minor = 1;
            m_pre = "";
            m_build = "";
        }

        Romver::Romver(int project, int major, int minor)
        {
            m_project = project;
            m_major = major;
            m_minor = minor;
            m_pre = "";
            m_build = "";
        }

        Romver::Romver(int project, int major, int minor, const std::string &pre, const std::string &build)
        {
            m_project = project;
            m_major = major;
            m_minor = minor;
            m_pre = pre;
            m_build = build;
        }

        Romver::Romver(const std::string &version)
        {
            const char* ptr = version.c_str();
            const char* end = ptr + version.size();

            auto parse_number = [&](uint16_t& out) -> bool
            {
                if (ptr == end || !std::isdigit(*ptr))
                    return false;

                uint32_t value = 0;

                if (*ptr == '0')
                {
                    ptr++;
                    out = 0;
                    return true;
                }

                while (ptr < end && std::isdigit(*ptr))
                {
                    value = value * 10 + (*ptr - '0');
                    ptr++;
                }

                out = static_cast<uint16_t>(value);
                return true;
            };

            if (!parse_number(m_project) || ptr == end || *ptr != '.') {
                spdlog::warn("Invalid version string '{}', default value (0.0.1) returned", version);
                m_project = 0;
                m_major = 0;
                m_minor = 1;
                return;
            }
            ptr++;

            if (!parse_number(m_major) || ptr == end || *ptr != '.') {
                spdlog::warn("Invalid version string '{}', default value (0.0.1) returned", version);
                m_project = 0;
                m_major = 0;
                m_minor = 1;
                return;
            };
            ptr++;

            if (!parse_number(m_minor)) {
                spdlog::warn("Invalid version string '{}', default value (0.0.1) returned", version);
                m_project = 0;
                m_major = 0;
                m_minor = 1;
                return;
            }

            m_pre = "";
            if (ptr < end && *ptr == '-')
            {
                ptr++;
                const char* start = ptr;

                while (ptr < end && *ptr != '+')
                    ptr++;

                m_pre.assign(start, ptr);
            }

            if (ptr < end && *ptr == '+')
            {
                ptr++;
                m_build.assign(ptr, end);
            }
        }

        Romver Romver::Parse(const std::string &version)
        {
            const char* ptr = version.c_str();
            const char* end = ptr + version.size();

            auto parse_number = [&](uint16_t& out) -> bool
            {
                if (ptr == end || !std::isdigit(*ptr))
                    return false;

                uint32_t value = 0;

                if (*ptr == '0')
                {
                    ptr++;
                    out = 0;
                    return true;
                }

                while (ptr < end && std::isdigit(*ptr))
                {
                    value = value * 10 + (*ptr - '0');
                    ptr++;
                }

                out = static_cast<uint16_t>(value);
                return true;
            };

            std::uint16_t project = 0;
            std::uint16_t major = 0;
            std::uint16_t minor = 1;

            if (!parse_number(project) || ptr == end || *ptr != '.') {
                spdlog::warn("Invalid version string '{}', default value (0.0.1) returned", version);
                return Romver();
            }
            ptr++;

            if (!parse_number(major) || ptr == end || *ptr != '.') {
                spdlog::warn("Invalid version string '{}', default value (0.0.1) returned", version);
                return Romver();
            };
            ptr++;

            if (!parse_number(minor)) {
                spdlog::warn("Invalid version string '{}', default value (0.0.1) returned", version);
                return Romver();
            }

            std::string pre = "";
            if (ptr < end && *ptr == '-')
            {
                ptr++;
                const char* start = ptr;

                while (ptr < end && *ptr != '+')
                    ptr++;

                pre.assign(start, ptr);
            }

            std::string build = "";
            if (ptr < end && *ptr == '+')
            {
                ptr++;
                build.assign(ptr, end);
            }

            return Romver(project, major, minor, pre, build);
        }

        const std::string Romver::to_string()
        {
            std::string version = std::to_string(m_project) + "." +
                                  std::to_string(m_major) + "." +
                                  std::to_string(m_minor);
            if (!m_pre.empty()) {
                version += "-" + m_pre;
            }
            if (!m_build.empty()) {
                version += "+" + m_build;
            }
            return version;
        }

        bool Romver::operator<(const Romver &other)
        {
            if (m_project != other.m_project) {
                return m_project < other.m_project;
            }
            if (m_major != other.m_major) {
                return m_major < other.m_major;
            }
            if (m_minor != other.m_minor) {
                return m_minor < other.m_minor;
            }

            const bool thisHasPre  = !m_pre.empty();
            const bool otherHasPre = !other.m_pre.empty();
            if (thisHasPre != otherHasPre) // Pre-relase always lower than release
                return thisHasPre;

            if (thisHasPre)
                return m_pre < other.m_pre;

            return false;
        }

        bool Romver::operator<=(const Romver &other)
        {
            return !(*this > other);
        }

        bool Romver::operator>(const Romver &other)
        {
            if (m_project != other.m_project) {
                return m_project > other.m_project;
            }
            if (m_major != other.m_major) {
                return m_major > other.m_major;
            }
            if (m_minor != other.m_minor) {
                return m_minor > other.m_minor;
            }

            const bool thisHasPre  = !m_pre.empty();
            const bool otherHasPre = !other.m_pre.empty();
            if (thisHasPre != otherHasPre) // Pre-relase always lower than release
                return otherHasPre;

            if (thisHasPre)
                return m_pre > other.m_pre;

            return false;
        }

        bool Romver::operator>=(const Romver &other)
        {
            return !(*this < other);
        }

        bool Romver::operator==(const Romver &other)
        {
            return (m_project == other.m_project && m_major == other.m_major &&
                m_minor == other.m_minor && m_pre == other.m_pre);
        }

        bool Romver::operator!=(const Romver &other)
        {
            return !(*this == other);
        }
    } // namespace impl
} // namespace atmo
