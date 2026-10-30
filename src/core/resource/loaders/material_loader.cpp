#include "material_loader.hpp"

#include "core/resource/auto_register_loader.hpp"
#include "project/file_system.hpp"
#include "server/rendering/rendering_server.hpp"

namespace atmo::core::resource
{
    std::shared_ptr<Material> MaterialLoader::load(const std::string &path)
    {
        if (!server::rendering::RenderingServer::Instance().isInitialized())
            throw LoadException("the RenderingServer is not initialized");

        std::string json;
        try {
            json = project::FileSystem::OpenFile(path).readAll();
        } catch (const std::exception &e) {
            throw LoadException("Material " + path + " could not be read: " + e.what());
        }

        std::string error;
        std::shared_ptr<Material> material = Material::FromJson(json, error);
        if (!material)
            throw LoadException("Material " + path + " is invalid: " + error);
        return material;
    }
} // namespace atmo::core::resource

ATMO_REGISTER_RESOURCE_LOADER(atmo::core::resource::Material, atmo::core::resource::MaterialLoader, .display_name = "Material", .extensions = { ".material" });
