#pragma once

#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "core/resource/subresource_registry.hpp"
#include "core/resource/subresources/subresource.hpp"
#include "glaze/glaze.hpp"
#include "project/file_system.hpp"

namespace atmo::core::resource
{
    struct SubResourceEntry {
        std::string id;
        std::string type;
        std::string data;
    };

    template <typename Main> struct ResourceFile {
        Main main;
        std::vector<SubResourceEntry> subresources;
    };

    template <typename Main> ResourceFile<Main> LoadResourceFile(const std::string &path)
    {
        auto file = project::FileSystem::OpenFile(path);
        std::string content = file.readAll();

        const bool is_binary = path.size() > 4 && path.compare(path.size() - 4, 4, ".bin") == 0;

        ResourceFile<Main> result;
        auto err = is_binary ? glz::read_beve(result, content) : glz::read_json(result, content);
        if (err) {
            throw std::runtime_error("Failed to parse resource file '" + path + "': " + glz::format_error(err, content));
        }

        return result;
    }

    template <typename Base, typename Main> std::shared_ptr<Base> ResolveSubResource(std::string_view path_and_id)
    {
        const auto hash_pos = path_and_id.find('#');
        if (hash_pos == std::string_view::npos) {
            throw std::runtime_error("Sub-resource address is missing '#id': " + std::string(path_and_id));
        }

        const std::string path(path_and_id.substr(0, hash_pos));
        const std::string_view id = path_and_id.substr(hash_pos + 1);

        ResourceFile<Main> file = LoadResourceFile<Main>(path);
        for (const auto &entry : file.subresources) {
            if (entry.id != id)
                continue;

            auto resource = SubResourceRegistry::Create<Base>(entry.type);
            if (resource)
                resource->deserialize(entry.data);
            return resource;
        }

        return nullptr;
    }
} // namespace atmo::core::resource
