#pragma once

//#include <ctre.hpp>
#include <string>

namespace atmo
{
    namespace impl
    {
        //inline constexpr auto ROMVER_REGEX =
        //    ctll::fixed_string{
        //        R"(^(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)(-([0-9A-Za-z]+([.\-][0-9A-Za-z]+)*))?(\+([0-9A-Za-z]+([.\-][0-9A-Za-z]+)*))?$)"
        //};

        class Romver {
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
