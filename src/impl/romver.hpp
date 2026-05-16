#pragma once

#include <string>

#include "glaze/glaze.hpp"

namespace atmo::impl
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

        const std::string toString();

    private:
        Romver(int project, int major, int minor);
        Romver(int project, int major, int minor, const std::string &pre, const std::string &build);

        std::uint16_t m_project;
        std::uint16_t m_major;
        std::uint16_t m_minor;
        std::string m_pre;
        std::string m_build;
    };
} // namespace atmo::impl

template <> struct glz::meta<atmo::impl::Romver> {
    using T = atmo::impl::Romver;
    static constexpr auto value = object(&T::toString);
};
