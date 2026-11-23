#include "helpers.hpp"

#include <algorithm>
#include <array>
#include <charconv>
#include <vector>

#include "SDL3/SDL_iostream.h"
#include "SDL3/SDL_surface.h"
#include "SDL3_image/SDL_image.h"
#include "core/ecs/entity_registry.hpp"
#include "core/engine.hpp"
#include "editor/editor_manager.hpp"
#include "editor/editors/scene_editor/scene_editor.hpp"
#include "mcp/mcp_server.hpp"
#include "meta/meta_registry.hpp"
#include "project/file_system.hpp"
#include "project/project_manager.hpp"

namespace atmo::mcp::tools
{
    namespace
    {
        std::optional<std::uint64_t> ParseId(std::string_view text)
        {
            std::uint64_t value = 0;
            auto [end, ec] = std::from_chars(text.data(), text.data() + text.size(), value);
            if (ec != std::errc() || end != text.data() + text.size() || text.empty())
                return std::nullopt;
            return value;
        }

        bool IsInside(flecs::entity entity, flecs::entity root)
        {
            for (flecs::entity current = entity; current.is_valid(); current = current.parent())
                if (current == root)
                    return true;
            return false;
        }

        core::ecs::entities::Entity ChildNamed(const core::ecs::entities::Entity &parent, std::string_view name)
        {
            for (auto &child : parent.getChildren())
                if (child.name() == name)
                    return child;
            return {};
        }
    } // namespace

    editor::EditorManager *EditorManagerOf(McpServer &server)
    {
        return server.engine().editor();
    }

    std::expected<editor::EditorManager *, std::string> RequireEditor(McpServer &server)
    {
        if (auto *manager = EditorManagerOf(server))
            return manager;
        return std::unexpected("This tool needs the editor (start atmo with --project <path>)");
    }

    std::filesystem::path ResolvePath(const std::string &path)
    {
        return editor::EditorManager::ResolveProjectPath(path);
    }

    std::string DisplayPath(const std::filesystem::path &path)
    {
        const std::filesystem::path root = std::filesystem::weakly_canonical(project::ProjectManager::GetCurrentProjectPath());
        std::error_code ec;
        const std::filesystem::path relative = std::filesystem::relative(path, root, ec);
        if (ec || relative.empty() || relative.native().starts_with(std::filesystem::path("..").native()))
            return path.generic_string();
        return relative.generic_string();
    }

    std::expected<std::shared_ptr<editor::Editor>, std::string> FindEditorTarget(editor::EditorManager &manager, const std::string &target)
    {
        if (auto id = ParseId(target)) {
            if (auto editor = manager.findEditor(static_cast<std::uint32_t>(*id)))
                return editor;
            return std::unexpected("No editor tab with id " + target);
        }
        if (auto editor = manager.findEditorByPath(ResolvePath(target)))
            return editor;
        return std::unexpected("No editor tab is open on " + target + " (see editor_list)");
    }

    glz::generic EditorJson(editor::EditorManager &manager, const editor::Editor &editor)
    {
        glz::generic::object_t json{
            { "id", static_cast<double>(editor.id()) },
            { "type", std::string(editor.getTypeName()) },
            { "dirty", editor.isDirty() },
            { "active", manager.activeEditor().get() == &editor },
            { "opened_by_mcp", editor.openedByMcp() },
        };
        if (editor.hasFilePath())
            json["path"] = DisplayPath(ResolvePath(*editor.filePath()));
        else
            json["path"] = nullptr;
        return json;
    }

    std::expected<SceneTarget, std::string> ResolveScene(McpServer &server, const std::optional<std::string> &scene)
    {
        auto *manager = EditorManagerOf(server);
        if (!manager) {
            SceneTarget target;
            target.world = &server.engine().getECS().getWorld();
            target.scene = server.engine().getECS().getCurrentScene();
            if (!target.scene)
                return std::unexpected("No scene is running");
            return target;
        }

        if (!scene || scene->empty()) {
            auto active = std::dynamic_pointer_cast<editor::SceneEditor>(manager->activeEditor());
            if (!active)
                return std::unexpected("Pass 'scene' (the scene file path), or focus a scene editor tab first");
            return SceneTarget{ &active->getSceneContext()->getWorld(), active->getSceneContext()->getScene(), active };
        }

        std::shared_ptr<editor::Editor> editor;
        if (auto found = FindEditorTarget(*manager, *scene))
            editor = *found;
        if (!editor) {
            const std::filesystem::path path = ResolvePath(*scene);
            if (!std::filesystem::exists(path))
                return std::unexpected("Scene file not found: " + *scene + " (create it with editor_open and create: true)");
            if (path.extension() != ".ascn")
                return std::unexpected("Not a scene file (.ascn): " + *scene);
            editor = manager->openEditor("Editor::SceneEditor", path.string());
            if (!editor)
                return std::unexpected("Could not open a scene editor for " + *scene);
            editor->setOpenedByMcp(true);
        }

        auto scene_editor = std::dynamic_pointer_cast<editor::SceneEditor>(editor);
        if (!scene_editor || !scene_editor->getSceneContext() || !scene_editor->getSceneContext()->getScene())
            return std::unexpected("Editor tab " + *scene + " is not a scene editor");
        return SceneTarget{ &scene_editor->getSceneContext()->getWorld(), scene_editor->getSceneContext()->getScene(), scene_editor };
    }

    std::expected<core::ecs::entities::Entity, std::string> ResolveEntity(const SceneTarget &target, std::string_view reference)
    {
        core::ecs::entities::Entity root = *target.scene;
        if (reference.empty() || reference == "/" || reference == ".")
            return root;

        if (auto id = ParseId(reference)) {
            flecs::entity entity(*target.world, static_cast<flecs::entity_t>(*id));
            if (entity.is_valid() && entity.is_alive() && IsInside(entity, root.getHandle()))
                return core::ecs::entities::Entity(entity);
            return std::unexpected(std::format("No entity with id {} in this scene", *id));
        }

        std::string_view rest = reference;
        if (rest.starts_with('/'))
            rest.remove_prefix(1);

        core::ecs::entities::Entity current = root;
        bool first = true;
        while (!rest.empty()) {
            const std::size_t slash = rest.find('/');
            const std::string_view segment = rest.substr(0, slash);
            rest = slash == std::string_view::npos ? std::string_view{} : rest.substr(slash + 1);

            core::ecs::entities::Entity next = ChildNamed(current, segment);
            if (!next.isAlive() && first && rest.empty())
                next = current.findChildRecursive(segment);
            if (!next.isAlive())
                return std::unexpected(std::format("No entity at '{}' (see scene_tree)", reference));
            current = next;
            first = false;
        }
        return current;
    }

    void MarkDirty(const SceneTarget &target)
    {
        if (target.editor)
            target.editor->markDirty();
    }

    std::string EntityPath(const SceneTarget &target, const core::ecs::entities::Entity &entity)
    {
        std::vector<std::string> parts;
        for (flecs::entity current = entity.getHandle(); current.is_valid() && current != target.scene->getHandle(); current = current.parent())
            parts.emplace_back(current.name().c_str());
        std::string path;
        for (auto it = parts.rbegin(); it != parts.rend(); ++it) {
            if (!path.empty())
                path += '/';
            path += *it;
        }
        return path;
    }

    std::string EntityType(const core::ecs::entities::Entity &entity)
    {
        if (const auto *base = entity.getHandle().try_get<core::components::EntityBase>())
            return base->type_name;
        return {};
    }

    glz::generic EntitySummary(const SceneTarget &target, const core::ecs::entities::Entity &entity)
    {
        return glz::generic::object_t{
            { "id", static_cast<double>(entity.getHandle().id()) },
            { "name", std::string(entity.name()) },
            { "type", EntityType(entity) },
            { "path", EntityPath(target, entity) },
        };
    }

    glz::generic::object_t ComponentsJson(flecs::entity entity)
    {
        glz::generic::object_t components;
        flecs::world world = entity.world();
        for (const auto &type : meta::MetaRegistry::Instance().all()) {
            if (!type.register_flecs || !type.resolve_flecs_id || !type.to_json)
                continue;
            const flecs::id_t id = type.resolve_flecs_id(world);
            if (id == 0 || !entity.has(id))
                continue;
            if (const void *data = entity.try_get(id))
                components[type.name] = type.to_json(data);
        }
        return components;
    }

    glz::generic::array_t ComponentNames(flecs::entity entity)
    {
        glz::generic::array_t names;
        for (const auto &[name, value] : ComponentsJson(entity)) names.emplace_back(name);
        return names;
    }

    std::optional<ComponentRef> FindComponent(flecs::world &world, std::string_view name)
    {
        const meta::TypeInfo *type = meta::MetaRegistry::Instance().find(name);
        if (!type || !type->register_flecs || !type->resolve_flecs_id || !type->from_json)
            return std::nullopt;
        return ComponentRef{ type, type->resolve_flecs_id(world) };
    }

    std::expected<void, std::string> SetComponent(flecs::entity entity, std::string_view name, const glz::generic &value)
    {
        flecs::world world = entity.world();
        auto component = FindComponent(world, name);
        if (!component || component->id == 0)
            return std::unexpected(std::format("Unknown component '{}' (see component_types)", name));

        void *data = entity.ensure(component->id);
        if (!data)
            return std::unexpected(std::format("Could not add component '{}'", name));

        const glz::generic merged = MergePatch(component->type->to_json(data), value);
        const std::string json = merged.dump().value_or("{}");
        if (!component->type->from_json(data, json))
            return std::unexpected(std::format("Invalid value for component '{}': {}", name, json));
        entity.modified(component->id);
        return {};
    }

    std::optional<std::string> ResolveEntityType(std::string_view type)
    {
        const auto entries = core::ecs::EntityRegistry::GetEntries();
        for (const auto &entry : entries)
            if (entry == type)
                return entry;
        const std::string suffix = std::format("::{}", type);
        for (const auto &entry : entries)
            if (entry.ends_with(suffix))
                return entry;
        return std::nullopt;
    }

    glz::generic MergePatch(const glz::generic &target, const glz::generic &patch)
    {
        if (!patch.is_object())
            return patch;
        glz::generic result = target.is_object() ? target : glz::generic::object_t{};
        for (const auto &[key, value] : patch.get_object()) {
            if (value.is_null()) {
                result.get_object().erase(key);
                continue;
            }
            const glz::generic &current = result.contains(key) ? result[key] : glz::generic{};
            result[key] = MergePatch(current, value);
        }
        return result;
    }

    std::string Base64(std::string_view bytes)
    {
        static constexpr std::string_view alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        std::string out;
        out.reserve((bytes.size() + 2) / 3 * 4);
        std::size_t i = 0;
        for (; i + 2 < bytes.size(); i += 3) {
            const auto n = (static_cast<std::uint32_t>(static_cast<unsigned char>(bytes[i])) << 16) |
                (static_cast<std::uint32_t>(static_cast<unsigned char>(bytes[i + 1])) << 8) | static_cast<unsigned char>(bytes[i + 2]);
            out += alphabet[(n >> 18) & 63];
            out += alphabet[(n >> 12) & 63];
            out += alphabet[(n >> 6) & 63];
            out += alphabet[n & 63];
        }
        if (i < bytes.size()) {
            std::uint32_t n = static_cast<std::uint32_t>(static_cast<unsigned char>(bytes[i])) << 16;
            if (i + 1 < bytes.size())
                n |= static_cast<std::uint32_t>(static_cast<unsigned char>(bytes[i + 1])) << 8;
            out += alphabet[(n >> 18) & 63];
            out += alphabet[(n >> 12) & 63];
            out += i + 1 < bytes.size() ? alphabet[(n >> 6) & 63] : '=';
            out += '=';
        }
        return out;
    }

    std::optional<std::string> EncodePngBase64(const server::rendering::TextureData &data)
    {
        if (data.size.x <= 0 || data.size.y <= 0 || data.pixels.size() < static_cast<std::size_t>(data.size.x) * data.size.y * 4)
            return std::nullopt;

        SDL_Surface *surface =
            SDL_CreateSurfaceFrom(data.size.x, data.size.y, SDL_PIXELFORMAT_RGBA32, const_cast<std::byte *>(data.pixels.data()), data.size.x * 4);
        if (!surface)
            return std::nullopt;

        SDL_IOStream *io = SDL_IOFromDynamicMem();
        std::optional<std::string> result;
        if (io && IMG_SavePNG_IO(surface, io, false)) {
            const Sint64 size = SDL_TellIO(io);
            auto *memory = static_cast<const char *>(SDL_GetPointerProperty(SDL_GetIOProperties(io), SDL_PROP_IOSTREAM_DYNAMIC_MEMORY_POINTER, nullptr));
            if (memory && size > 0)
                result = Base64(std::string_view(memory, static_cast<std::size_t>(size)));
        }
        if (io)
            SDL_CloseIO(io);
        SDL_DestroySurface(surface);
        return result;
    }
} // namespace atmo::mcp::tools
