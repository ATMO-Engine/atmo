#include "impl/romver.hpp"

#include "spdlog/spdlog.h"

#include <minwindef.h>
#include <regex>
#include <string>

namespace atmo
{
    namespace core
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
                std::smatch match;

                if (!std::regex_match(version, match, m_regex)) {
                    spdlog::warn("Version string can't be parsed, default value (0.0.1) returned");
                    m_project = 0;
                    m_major = 0;
                    m_minor = 1;
                    return;
                }

                m_project = std::stoi(match[1].str());
                m_major = std::stoi(match[2].str());
                m_minor = std::stoi(match[3].str());

                // Combine suffix if present
                if (match[4].matched)
                    m_pre = match[4].str();

                if (match[5].matched)
                {
                    m_build += match[5].str();
                }
            }

            Romver Romver::Parse(const std::string &version)
            {
                std::smatch match;

                if (!std::regex_match(version, match, m_regex)) {
                    spdlog::warn("Version string can't be parsed, default value (0.0.1) returned");
                    return Romver();
                }

                std::uint16_t project = std::stoi(match[1].str());
                std::uint16_t major = std::stoi(match[2].str());
                std::uint16_t minor = std::stoi(match[3].str());

                std::string pre = "";
                std::string build = "";

                if (match[4].matched)
                    pre = match[4].str();

                if (match[5].matched)
                {
                    build += match[5].str();
                }

                return Romver(project, major, minor, pre, build);
            }

            const std::string Romver::str()
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
    } // namespace core
} // namespace atmo
