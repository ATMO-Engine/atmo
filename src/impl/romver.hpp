#pragma once

#include <string>

namespace atmo
{
    namespace impl
    {
        class Romver
        {
        public:
            Romver();
            Romver(const std::string &version);

            ~Romver() = default;

            static Romver Parse(const std::string &version);

            bool operator<(const Romver &other);
            bool operator<=(const Romver &other);
            bool operator>(const Romver &other);
            bool operator>=(const Romver &other);
            bool operator==(const Romver &other);
            bool operator!=(const Romver &other);

            const std::string to_string();

        private:
            Romver(int project, int major, int minor);
            Romver(int project, int major, int minor, const std::string &pre, const std::string &build);

            std::uint16_t m_project;
            std::uint16_t m_major;
            std::uint16_t m_minor;
            std::string m_pre;
            std::string m_build;
        };
    } // namespace impl
} // namespace atmo
