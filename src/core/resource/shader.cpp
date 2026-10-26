#include "shader.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <fstream>
#include <sstream>
#include <system_error>

#include "glaze/glaze.hpp"
#include "project/file_system.hpp"
#include "server/rendering/rendering_server.hpp"
#include "spdlog/spdlog.h"

#if !defined(ATMO_EXPORT)
#include "project/project_manager.hpp"
#include "server/rendering/shader/shader_compiler.hpp"
#endif

namespace atmo::core::resource
{
    namespace detail
    {
        struct SerializedParam {
            std::string name;
            std::string type;
            std::uint32_t offset = 0;
            std::vector<float> range;
        };

        struct SerializedCode {
            std::string target;
            std::vector<std::uint8_t> code;
        };

        struct SerializedShader {
            std::uint32_t version = 1;
            std::string name;
            std::string kind;
            std::string entry_point;
            std::uint32_t size = 0;
            std::vector<SerializedParam> params;
            std::vector<SerializedCode> code;
        };
    } // namespace detail

    namespace
    {
        namespace rendering = server::rendering;

        constexpr std::uint32_t serialized_version = 1;
        using detail::SerializedCode;
        using detail::SerializedParam;
        using detail::SerializedShader;

        constexpr std::array<std::pair<rendering::ShaderTarget, std::string_view>, 4> target_names{ {
            { rendering::ShaderTarget::SpirV, "spirv" },
            { rendering::ShaderTarget::Msl, "msl" },
            { rendering::ShaderTarget::MetalLib, "metallib" },
            { rendering::ShaderTarget::Dxil, "dxil" },
        } };

        std::string_view TargetName(rendering::ShaderTarget target)
        {
            auto it = std::ranges::find(target_names, target, &std::pair<rendering::ShaderTarget, std::string_view>::first);
            return it == target_names.end() ? "none" : it->second;
        }

        std::optional<rendering::ShaderTarget> ParseTarget(std::string_view name)
        {
            auto it = std::ranges::find(target_names, name, &std::pair<rendering::ShaderTarget, std::string_view>::second);
            if (it == target_names.end())
                return std::nullopt;
            return it->first;
        }

        std::vector<Shader *> &LiveShaders()
        {
            static std::vector<Shader *> shaders;
            return shaders;
        }

#if !defined(ATMO_EXPORT)
        constexpr std::array export_targets{ rendering::ShaderTarget::SpirV, rendering::ShaderTarget::Msl };
        constexpr auto reload_interval = std::chrono::milliseconds(500);

        rendering::ShaderCompiler &Compiler()
        {
            static rendering::ShaderCompiler compiler;
            return compiler;
        }

        std::vector<rendering::ShaderTarget> LoadTargets()
        {
            const rendering::ShaderTarget target = rendering::RenderingServer::Instance().shaderTarget();
            if (rendering::ShaderCompiler::Supports(target))
                return { target };
            return { rendering::ShaderTarget::SpirV };
        }

        std::filesystem::path SourceFile(const std::string &path)
        {
            std::error_code error;
            std::filesystem::path resolved = project::FileSystem::ResolvePath(path);
            if (std::filesystem::is_regular_file(resolved, error))
                return std::filesystem::absolute(resolved, error);
            return {};
        }

        std::vector<project::ProjectManager::GeneratedFile> CompileProjectShaders(const std::filesystem::path &root)
        {
            std::vector<project::ProjectManager::GeneratedFile> files;
            std::error_code error;
            for (const auto &entry : std::filesystem::recursive_directory_iterator(root, error)) {
                if (!entry.is_regular_file() || entry.path().extension() != ".slang")
                    continue;
                const std::string relative = std::filesystem::relative(entry.path(), root).generic_string();
                std::ifstream in(entry.path(), std::ios::binary);
                std::stringstream code;
                code << in.rdbuf();

                auto desc = Shader::Compile(entry.path().stem().string(), relative, code.str(), entry.path().parent_path(), export_targets);
                if (!desc) {
                    if (desc.error().find("defines no struct") != std::string::npos)
                        spdlog::debug("Export: {} is not a material, packed as source only", relative);
                    else
                        spdlog::error("Export: shader {} failed to compile:\n{}", relative, desc.error());
                    continue;
                }
                files.push_back({ .path = relative + std::string(Shader::compiled_suffix), .data = Shader::Serialize(*desc) });
                spdlog::info("Export: compiled shader {}", relative);
            }
            return files;
        }

        const bool export_processor_registered = [] {
            project::ProjectManager::RegisterExportProcessor(CompileProjectShaders);
            return true;
        }();
#endif
    } // namespace

    Shader::Shader(std::string path, rendering::ShaderDesc desc) : m_path(std::move(path)), m_kind(desc.kind), m_layout(desc.layout)
    {
#if !defined(ATMO_EXPORT)
        m_source = SourceFile(m_path);
        std::error_code error;
        if (!m_source.empty())
            m_mtime = std::filesystem::last_write_time(m_source, error);
#endif
        m_rid = rendering::RenderingServer::Instance().shaderCreate(std::move(desc));
        LiveShaders().push_back(this);
    }

    Shader::~Shader()
    {
        std::erase(LiveShaders(), this);
        rendering::RenderingServer::Instance().shaderFree(m_rid);
    }

    rendering::ShaderRid Shader::rid() const noexcept
    {
        return m_rid;
    }

    rendering::ShaderKind Shader::kind() const noexcept
    {
        return m_kind;
    }

    const rendering::ShaderParamLayout &Shader::layout() const noexcept
    {
        return m_layout;
    }

    const std::string &Shader::path() const noexcept
    {
        return m_path;
    }

    std::uint64_t Shader::version() const noexcept
    {
        return m_version;
    }

    std::vector<std::byte> Shader::Serialize(const rendering::ShaderDesc &desc)
    {
        SerializedShader serialized{
            .name = desc.name,
            .kind = std::string(rendering::ShaderKindName(desc.kind)),
            .entry_point = desc.entry_point,
            .size = desc.layout.size,
        };
        for (const rendering::ShaderParam &param : desc.layout.params) {
            SerializedParam out{ .name = param.name, .type = std::string(rendering::ShaderParamTypeName(param.type)), .offset = param.offset };
            if (param.range)
                out.range = { param.range->at(0), param.range->at(1) };
            serialized.params.push_back(std::move(out));
        }
        for (const rendering::ShaderCode &code : desc.code) {
            const auto *bytes = reinterpret_cast<const std::uint8_t *>(code.code.data());
            serialized.code.push_back({ .target = std::string(TargetName(code.target)), .code = std::vector<std::uint8_t>(bytes, bytes + code.code.size()) });
        }

        std::string buffer;
        if (glz::write_beve(serialized, buffer))
            return {};
        const auto *begin = reinterpret_cast<const std::byte *>(buffer.data());
        return { begin, begin + buffer.size() };
    }

    std::optional<rendering::ShaderDesc> Shader::Deserialize(std::string_view data)
    {
        SerializedShader serialized;
        if (glz::read_beve(serialized, data) || serialized.version != serialized_version)
            return std::nullopt;

        std::optional<rendering::ShaderKind> kind = rendering::ParseShaderKind(serialized.kind);
        if (!kind)
            return std::nullopt;

        rendering::ShaderDesc desc{ .name = serialized.name, .kind = *kind, .entry_point = serialized.entry_point };
        desc.layout.size = serialized.size;
        for (const SerializedParam &param : serialized.params) {
            std::optional<rendering::ShaderParamType> type = rendering::ParseShaderParamType(param.type);
            if (!type)
                return std::nullopt;
            rendering::ShaderParam out{ .name = param.name, .type = *type, .offset = param.offset };
            if (param.range.size() == 2)
                out.range = std::array{ param.range[0], param.range[1] };
            desc.layout.params.push_back(std::move(out));
        }
        for (const SerializedCode &code : serialized.code) {
            if (std::optional<rendering::ShaderTarget> target = ParseTarget(code.target)) {
                const auto *bytes = reinterpret_cast<const std::byte *>(code.code.data());
                desc.code.push_back({ .target = *target, .code = std::vector<std::byte>(bytes, bytes + code.code.size()) });
            }
        }
        return desc;
    }

#if !defined(ATMO_EXPORT)
    std::expected<rendering::ShaderDesc, std::string> Shader::Compile(
        std::string_view name, std::string_view path, const std::string &code, const std::filesystem::path &directory,
        std::span<const rendering::ShaderTarget> targets)
    {
        rendering::ShaderSource source{ .name = std::string(name), .path = std::string(path), .code = code };
        if (!directory.empty())
            source.search_paths.push_back(directory.string());

        auto compiled = Compiler().compileMaterial(source, targets);
        if (!compiled)
            return std::unexpected(compiled.error());
        if (!compiled->diagnostics.empty())
            spdlog::warn("Shader {}:\n{}", path, compiled->diagnostics);
        return std::move(compiled->desc);
    }

    std::expected<rendering::ShaderDesc, std::string> Shader::CompileFile(const std::string &path)
    {
        std::string code;
        try {
            code = project::FileSystem::OpenFile(path).readAll();
        } catch (const std::exception &e) {
            return std::unexpected(std::string(e.what()));
        }
        const std::filesystem::path source = SourceFile(path);
        return Compile(std::filesystem::path(path).stem().string(), path, code, source.empty() ? std::filesystem::path() : source.parent_path(), LoadTargets());
    }

    bool Shader::reload()
    {
        auto desc = CompileFile(m_path);
        if (!desc) {
            spdlog::error("Shader {} failed to compile, keeping the previous version:\n{}", m_path, desc.error());
            return false;
        }
        m_kind = desc->kind;
        m_layout = desc->layout;
        ++m_version;
        rendering::RenderingServer::Instance().shaderUpdate(m_rid, std::move(*desc));
        spdlog::info("Reloaded shader {}", m_path);
        return true;
    }

    std::size_t Shader::ReloadChanged()
    {
        static auto last_check = std::chrono::steady_clock::time_point{};
        const auto now = std::chrono::steady_clock::now();
        if (now - last_check < reload_interval)
            return 0;
        last_check = now;

        std::size_t reloaded = 0;
        for (Shader *shader : LiveShaders()) {
            if (shader->m_source.empty())
                continue;
            std::error_code error;
            const auto mtime = std::filesystem::last_write_time(shader->m_source, error);
            if (error || mtime == shader->m_mtime)
                continue;
            shader->m_mtime = mtime;
            if (shader->reload())
                ++reloaded;
        }
        return reloaded;
    }
#endif
} // namespace atmo::core::resource
