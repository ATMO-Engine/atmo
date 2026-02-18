#pragma once

#include <regex>
#include <string>

namespace atmo
{
    namespace core
    {
        namespace impl
        {
            class Romver {
                public:
                    Romver(const std::string &version);

                    ~Romver();

                    static Romver Parse(const std::string &version);

                    bool operator<(const Romver &other);
                    bool operator<=(const Romver &other);
                    bool operator>(const Romver &other);
                    bool operator>=(const Romver &other);
                    bool operator==(const Romver &other);
                    bool operator!=(const Romver &other);

                    const std::string str();
                private:
                    Romver();
                    Romver(int project, int major, int minor);
                    Romver(int project, int major, int minor, const std::string &pre, const std::string &build);

                    std::uint16_t m_project;
                    std::uint16_t m_major;
                    std::uint16_t m_minor;
                    std::string m_pre;
                    std::string m_build;

                    inline const static std::regex m_regex = std::regex(
                        R"(^(0|[1-9]\d*)\.(0|[1-9]\d*)\.(0|[1-9]\d*)(?:-([0-9A-Za-z]+(?:[.-][0-9A-Za-z]+)*))?(?:\+([0-9A-Za-z]+(?:[.-][0-9A-Za-z]+)*))?$)"
                    );
            };
        } // namespace impl
    } // namespace core
} // namespace atmo
