#pragma once

#include <cstddef>
#include <cstdint>
#include <expected>
#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "server/rendering/material_types.hpp"
#include "server/rendering/rid.hpp"

namespace atmo::core::resource
{
    class Shader
    {
    public:
        static constexpr std::string_view compiled_suffix = ".compiled";

        Shader(std::string path, server::rendering::ShaderDesc desc);
        ~Shader();

        Shader(const Shader &) = delete;
        Shader &operator=(const Shader &) = delete;

        [[nodiscard]] server::rendering::ShaderRid rid() const noexcept;
        [[nodiscard]] server::rendering::ShaderKind kind() const noexcept;
        [[nodiscard]] const server::rendering::ShaderParamLayout &layout() const noexcept;
        [[nodiscard]] const std::string &path() const noexcept;
        [[nodiscard]] std::uint64_t version() const noexcept;

        [[nodiscard]] static std::vector<std::byte> Serialize(const server::rendering::ShaderDesc &desc);
        [[nodiscard]] static std::optional<server::rendering::ShaderDesc> Deserialize(std::string_view data);

#if !defined(ATMO_EXPORT)
        [[nodiscard]] static std::expected<server::rendering::ShaderDesc, std::string> Compile(
            std::string_view name, std::string_view path, const std::string &code, const std::filesystem::path &directory,
            std::span<const server::rendering::ShaderTarget> targets);
        [[nodiscard]] static std::expected<server::rendering::ShaderDesc, std::string> CompileFile(const std::string &path);

        bool reload();
        static std::size_t ReloadChanged();
#endif

    private:
        std::string m_path;
        server::rendering::ShaderRid m_rid;
        server::rendering::ShaderKind m_kind;
        server::rendering::ShaderParamLayout m_layout;
        std::uint64_t m_version = 0;
#if !defined(ATMO_EXPORT)
        std::filesystem::path m_source;
        std::filesystem::file_time_type m_mtime;
#endif
    };
} // namespace atmo::core::resource
