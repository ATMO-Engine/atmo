#include "shader_loader.hpp"

#include <optional>

#include "core/resource/auto_register_loader.hpp"
#include "project/file_system.hpp"
#include "server/rendering/rendering_server.hpp"

namespace atmo::core::resource
{
    std::shared_ptr<Shader> ShaderLoader::load(const std::string &path)
    {
        if (!server::rendering::RenderingServer::Instance().isInitialized())
            throw LoadException("the RenderingServer is not initialized");

#if !defined(ATMO_EXPORT)
        auto desc = Shader::CompileFile(path);
        if (!desc)
            throw LoadException("Shader " + path + " failed to compile:\n" + desc.error());
        return std::make_shared<Shader>(path, std::move(*desc));
#else
        std::string data;
        try {
            data = project::FileSystem::OpenFile(path + std::string(Shader::compiled_suffix)).readAll();
        } catch (const std::exception &e) {
            throw LoadException("Shader " + path + " was not compiled by the export: " + e.what());
        }
        std::optional<server::rendering::ShaderDesc> desc = Shader::Deserialize(data);
        if (!desc)
            throw LoadException("Shader " + path + " has an unreadable compiled file");
        return std::make_shared<Shader>(path, std::move(*desc));
#endif
    }
} // namespace atmo::core::resource

ATMO_REGISTER_RESOURCE_LOADER(atmo::core::resource::Shader, atmo::core::resource::ShaderLoader, .display_name = "Shader", .extensions = { ".slang" });
