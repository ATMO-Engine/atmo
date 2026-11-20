#include <filesystem>
#include <format>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include "core/ecs/entity_registry.hpp"
#include "core/engine.hpp"
#include "core/scene/scene_loader.hpp"
#include "editor/editor_manager.hpp"
#include "editor/editors/scene_editor/scene_editor.hpp"
#include "mcp/mcp_server.hpp"
#include "mcp/tools/helpers.hpp"
#include "mcp/tools/tools.hpp"
#include "meta/meta_registry.hpp"
#include "project/file.hpp"
#include "project/file_system.hpp"

namespace atmo::mcp::tools
{
    namespace scene_args
    {
        struct ComponentTypesArgs {
            std::optional<std::string> name;
        };

        struct SceneTreeArgs {
            std::optional<std::string> scene;
            std::optional<std::string> entity;
            std::optional<int> depth;
        };

        struct EntityArgs {
            std::optional<std::string> scene;
            std::string entity;
        };

        struct EntityCreateArgs {
            std::optional<std::string> scene;
            std::string type;
            std::optional<std::string> parent;
            std::optional<std::string> name;
            std::optional<std::map<std::string, glz::generic>> components;
        };

        struct EntityRenameArgs {
            std::optional<std::string> scene;
            std::string entity;
            std::string name;
        };

        struct EntityReparentArgs {
            std::optional<std::string> scene;
            std::string entity;
            std::string parent;
        };

        struct ComponentSetArgs {
            std::optional<std::string> scene;
            std::string entity;
            std::string component;
            glz::generic value;
        };

        struct ComponentRemoveArgs {
            std::optional<std::string> scene;
            std::string entity;
            std::string component;
        };

        struct PrefabInstantiateArgs {
            std::optional<std::string> scene;
            std::string path;
            std::optional<std::string> parent;
        };

        struct PrefabSaveArgs {
            std::optional<std::string> scene;
            std::string entity;
            std::string path;
        };

        struct SceneSaveArgs {
            std::optional<std::string> scene;
            std::optional<std::string> path;
        };

        constexpr const char *scene_description = "Scene file path (editor) or omitted (running game, or the focused scene editor tab)";
        constexpr const char *entity_description = "Entity path below the scene root ('Player/Sprite'), its numeric id, or '/' for the scene root";
    } // namespace scene_args
} // namespace atmo::mcp::tools

using namespace atmo::mcp::tools::scene_args;

template <> struct glz::json_schema<ComponentTypesArgs> {
    schema name{ .description = "Only describe this component (e.g. 'Transform2d')" };
};
template <> struct glz::json_schema<SceneTreeArgs> {
    schema scene{ .description = scene_description };
    schema entity{ .description = "Subtree root (default: the scene root)" };
    schema depth{ .description = "Maximum depth (default 16)" };
};
template <> struct glz::json_schema<EntityArgs> {
    schema scene{ .description = scene_description };
    schema entity{ .description = entity_description };
};
template <> struct glz::json_schema<EntityCreateArgs> {
    schema scene{ .description = scene_description };
    schema type{ .description = "Entity type, full ('Entity::Entity2d::Sprite2d') or short ('Sprite2d'); see entity_types" };
    schema parent{ .description = "Parent entity (default: the scene root)" };
    schema name{ .description = "Entity name; made unique among its siblings" };
    schema components{ .description = "Component values by component name, merged into the defaults (JSON merge patch)" };
};
template <> struct glz::json_schema<EntityRenameArgs> {
    schema scene{ .description = scene_description };
    schema entity{ .description = entity_description };
    schema name{ .description = "New name; made unique among its siblings" };
};
template <> struct glz::json_schema<EntityReparentArgs> {
    schema scene{ .description = scene_description };
    schema entity{ .description = entity_description };
    schema parent{ .description = "New parent entity ('/' for the scene root)" };
};
template <> struct glz::json_schema<ComponentSetArgs> {
    schema scene{ .description = scene_description };
    schema entity{ .description = entity_description };
    schema component{ .description = "Component name (see component_types); added when missing" };
    schema value{ .description = "Field values, merged into the current value (JSON merge patch)" };
};
template <> struct glz::json_schema<ComponentRemoveArgs> {
    schema scene{ .description = scene_description };
    schema entity{ .description = entity_description };
    schema component{ .description = "Component name" };
};
template <> struct glz::json_schema<PrefabInstantiateArgs> {
    schema scene{ .description = scene_description };
    schema path{ .description = "Prefab file (.ascn)" };
    schema parent{ .description = "Parent entity (default: the scene root)" };
};
template <> struct glz::json_schema<PrefabSaveArgs> {
    schema scene{ .description = scene_description };
    schema entity{ .description = entity_description };
    schema path{ .description = "Destination file; .ascn is added when missing" };
};
template <> struct glz::json_schema<SceneSaveArgs> {
    schema scene{ .description = "Scene file path or editor id (default: the focused scene editor tab)" };
    schema path{ .description = "Save to this path instead (save as)" };
};

namespace atmo::mcp::tools
{
    namespace
    {
        glz::generic TreeJson(const SceneTarget &target, const core::ecs::entities::Entity &entity, int depth)
        {
            glz::generic node = EntitySummary(target, entity);
            node.get_object().erase("path");
            node["components"] = ComponentNames(entity.getHandle());
            glz::generic::array_t children;
            for (const auto &child : entity.getChildren()) {
                if (depth > 0)
                    children.push_back(TreeJson(target, child, depth - 1));
            }
            if (depth > 0)
                node["children"] = std::move(children);
            else if (!entity.getChildren().empty())
                node["children_count"] = static_cast<double>(entity.getChildren().size());
            return node;
        }

        glz::generic TypeJson(const meta::TypeInfo &type)
        {
            glz::generic::array_t fields;
            for (const auto &field : type.fields) {
                glz::generic::object_t json{ { "name", field.name ? field.name : "" }, { "widget", field.widget ? field.widget : "" } };
                if (field.read_only)
                    json["read_only"] = true;
                if (field.has_range)
                    json["range"] = glz::generic::array_t{ static_cast<double>(field.range_min), static_cast<double>(field.range_max) };
                if (field.tooltip)
                    json["tooltip"] = field.tooltip;
                fields.push_back(std::move(json));
            }
            glz::generic::object_t json{ { "name", type.name }, { "fields", std::move(fields) } };
            if (type.category)
                json["category"] = type.category;
            return json;
        }

        std::string SceneFilePath(const std::string &path)
        {
            std::filesystem::path resolved = ResolvePath(path);
            if (resolved.extension() != ".ascn")
                resolved += ".ascn";
            return resolved.string();
        }

        template <class Args> std::expected<std::pair<SceneTarget, core::ecs::entities::Entity>, std::string> Resolve(McpServer &server, const Args &args)
        {
            auto target = ResolveScene(server, args.scene);
            if (!target)
                return std::unexpected(target.error());
            auto entity = ResolveEntity(*target, args.entity);
            if (!entity)
                return std::unexpected(entity.error());
            return std::pair{ *target, *entity };
        }
    } // namespace

    void RegisterSceneTools(McpServer &server)
    {
        auto &tools = server.tools();

        tools.add<NoArgs>("entity_types", "List the entity types that entity_create accepts.", [](const NoArgs &, const Reply &reply) {
            glz::generic::array_t types;
            for (const auto &entry : core::ecs::EntityRegistry::GetEntries())
                if (!core::ecs::EntityRegistry::IsAbstract(entry))
                    types.emplace_back(entry);
            reply.json(glz::generic::object_t{ { "types", std::move(types) } });
        });

        tools.add<ComponentTypesArgs>(
            "component_types",
            "List the components that can be read and set on entities, with their fields.",
            [](const ComponentTypesArgs &args, const Reply &reply) {
                glz::generic::array_t types;
                for (const auto &type : meta::MetaRegistry::Instance().all()) {
                    if (!type.register_flecs)
                        continue;
                    if (args.name && *args.name != type.name)
                        continue;
                    types.push_back(TypeJson(type));
                }
                if (args.name && types.empty())
                    return reply.error("Unknown component: " + *args.name);
                reply.json(glz::generic::object_t{ { "components", std::move(types) } });
            });

        tools.add<SceneTreeArgs>(
            "scene_tree",
            "Show the entity tree of a scene: id, name, type and component names of every entity.",
            [&server](const SceneTreeArgs &args, const Reply &reply) {
                auto target = ResolveScene(server, args.scene);
                if (!target)
                    return reply.error(target.error());
                auto root = ResolveEntity(*target, args.entity.value_or("/"));
                if (!root)
                    return reply.error(root.error());
                reply.json(TreeJson(*target, *root, args.depth.value_or(16)));
            });

        tools.add<EntityArgs>("entity_get", "Get an entity with all its component values.", [&server](const EntityArgs &args, const Reply &reply) {
            auto resolved = Resolve(server, args);
            if (!resolved)
                return reply.error(resolved.error());
            auto &[target, entity] = *resolved;
            glz::generic json = EntitySummary(target, entity);
            json["components"] = ComponentsJson(entity.getHandle());
            glz::generic::array_t children;
            for (const auto &child : entity.getChildren()) children.push_back(EntitySummary(target, child));
            json["children"] = std::move(children);
            if (entity.getHandle() != target.scene->getHandle()) {
                const std::string parent = EntityPath(target, entity.getParent());
                json["parent"] = parent.empty() ? std::string("/") : parent;
            }
            reply.json(json);
        });

        tools.add<EntityCreateArgs>("entity_create", "Create an entity in a scene.", [&server](const EntityCreateArgs &args, const Reply &reply) {
            auto target = ResolveScene(server, args.scene);
            if (!target)
                return reply.error(target.error());
            auto parent = ResolveEntity(*target, args.parent.value_or("/"));
            if (!parent)
                return reply.error(parent.error());
            auto type = ResolveEntityType(args.type);
            if (!type || core::ecs::EntityRegistry::IsAbstract(*type))
                return reply.error("Unknown or abstract entity type: " + args.type + " (see entity_types)");

            auto entity = core::ecs::EntityRegistry::CreateIn(target->world, *type);
            if (!entity)
                return reply.error("Could not create a " + *type);
            if (args.name)
                entity->rename(parent->uniqueChildName(*args.name));

            std::vector<std::string> errors;
            if (args.components)
                for (const auto &[name, value] : *args.components)
                    if (auto set = SetComponent(entity->getHandle(), name, value); !set)
                        errors.push_back(set.error());

            entity->setParent(*parent);
            MarkDirty(*target);

            glz::generic json = EntitySummary(*target, *entity);
            if (!errors.empty()) {
                glz::generic::array_t list;
                for (auto &error : errors) list.emplace_back(error);
                json["errors"] = std::move(list);
            }
            reply.json(json);
        });

        tools.add<EntityArgs>("entity_delete", "Delete an entity and its children.", [&server](const EntityArgs &args, const Reply &reply) {
            auto resolved = Resolve(server, args);
            if (!resolved)
                return reply.error(resolved.error());
            auto &[target, entity] = *resolved;
            if (entity.getHandle() == target.scene->getHandle())
                return reply.error("The scene root cannot be deleted");
            const std::string path = EntityPath(target, entity);
            entity.destroy();
            MarkDirty(target);
            reply.json(glz::generic::object_t{ { "deleted", path } });
        });

        tools.add<EntityRenameArgs>("entity_rename", "Rename an entity.", [&server](const EntityRenameArgs &args, const Reply &reply) {
            auto resolved = Resolve(server, args);
            if (!resolved)
                return reply.error(resolved.error());
            auto &[target, entity] = *resolved;
            if (entity.getHandle() == target.scene->getHandle())
                return reply.error("The scene root cannot be renamed");
            if (entity.name() != args.name)
                entity.rename(entity.getParent().uniqueChildName(args.name));
            MarkDirty(target);
            reply.json(EntitySummary(target, entity));
        });

        tools.add<EntityReparentArgs>("entity_reparent", "Move an entity under another parent.", [&server](const EntityReparentArgs &args, const Reply &reply) {
            auto resolved = Resolve(server, args);
            if (!resolved)
                return reply.error(resolved.error());
            auto &[target, entity] = *resolved;
            auto parent = ResolveEntity(target, args.parent);
            if (!parent)
                return reply.error(parent.error());
            if (entity.getHandle() == target.scene->getHandle())
                return reply.error("The scene root cannot be moved");
            for (flecs::entity current = parent->getHandle(); current.is_valid(); current = current.parent())
                if (current == entity.getHandle())
                    return reply.error("An entity cannot be moved under itself or one of its children");
            if (entity.getParent() == *parent)
                return reply.json(EntitySummary(target, entity));

            entity.getParent().getSignal<core::ecs::entities::Entity>("child_removed").emit(entity);
            const std::string unique = parent->uniqueChildName(entity.name());
            if (unique != entity.name())
                entity.rename(unique);
            entity.setParent(*parent);
            MarkDirty(target);
            reply.json(EntitySummary(target, entity));
        });

        tools.add<ComponentSetArgs>(
            "component_set",
            "Set fields of a component on an entity (the component is added when missing). Read the current value with entity_get.",
            [&server](const ComponentSetArgs &args, const Reply &reply) {
                auto resolved = Resolve(server, args);
                if (!resolved)
                    return reply.error(resolved.error());
                auto &[target, entity] = *resolved;
                if (auto set = SetComponent(entity.getHandle(), args.component, args.value); !set)
                    return reply.error(set.error());
                MarkDirty(target);
                flecs::world world = entity.getHandle().world();
                auto component = FindComponent(world, args.component);
                reply.json(component->type->to_json(entity.getHandle().try_get(component->id)));
            });

        tools.add<ComponentRemoveArgs>(
            "component_remove", "Remove a component from an entity.", [&server](const ComponentRemoveArgs &args, const Reply &reply) {
                auto resolved = Resolve(server, args);
                if (!resolved)
                    return reply.error(resolved.error());
                auto &[target, entity] = *resolved;
                flecs::world world = entity.getHandle().world();
                auto component = FindComponent(world, args.component);
                if (!component || !entity.getHandle().has(component->id))
                    return reply.error("The entity has no component " + args.component);
                entity.getHandle().remove(component->id);
                MarkDirty(target);
                reply.json(glz::generic::object_t{ { "removed", args.component } });
            });

        tools.add<PrefabInstantiateArgs>(
            "prefab_instantiate", "Instantiate a prefab file under an entity.", [&server](const PrefabInstantiateArgs &args, const Reply &reply) {
                auto target = ResolveScene(server, args.scene);
                if (!target)
                    return reply.error(target.error());
                auto parent = ResolveEntity(*target, args.parent.value_or("/"));
                if (!parent)
                    return reply.error(parent.error());
                auto instance = core::scene::SceneLoader::InstantiatePrefab(ResolvePath(args.path).string(), *parent);
                if (!instance)
                    return reply.error("Could not instantiate " + args.path + " (see logs_read)");
                MarkDirty(*target);
                reply.json(EntitySummary(*target, *instance));
            });

        tools.add<PrefabSaveArgs>(
            "prefab_save", "Save an entity and its children as a prefab file.", [&server](const PrefabSaveArgs &args, const Reply &reply) {
                auto resolved = Resolve(server, args);
                if (!resolved)
                    return reply.error(resolved.error());
                auto &[target, entity] = *resolved;
                auto data = glz::write<glz::opts{ .prettify = true }>(entity.serialize());
                if (!data)
                    return reply.error("Could not serialize the entity");
                const std::string path = SceneFilePath(args.path);
                std::filesystem::create_directories(std::filesystem::path(path).parent_path());
                project::File file = project::FileSystem::OpenFile(path, std::ios::out);
                file.write(data->c_str(), data->size());
                reply.json(glz::generic::object_t{ { "saved", DisplayPath(path) } });
            });

        if (server.mode() != McpServer::Mode::Editor)
            return;

        tools.add<EntityArgs>(
            "entity_select", "Select an entity in its scene editor (tree and inspector).", [&server](const EntityArgs &args, const Reply &reply) {
                auto resolved = Resolve(server, args);
                if (!resolved)
                    return reply.error(resolved.error());
                auto &[target, entity] = *resolved;
                server.engine().editor()->activateEditor(target.editor);
                target.editor->setSelectedEntity(entity.getHandle());
                reply.json(EntitySummary(target, entity));
            });

        tools.add<SceneSaveArgs>(
            "scene_save", "Save a scene editor tab to its file, or to a new path.", [&server](const SceneSaveArgs &args, const Reply &reply) {
                auto target = ResolveScene(server, args.scene);
                if (!target)
                    return reply.error(target.error());
                if (args.path) {
                    target->editor->saveAs(SceneFilePath(*args.path));
                } else {
                    if (!target->editor->hasFilePath())
                        return reply.error("The scene has no file yet; pass 'path'");
                    target->editor->save();
                    target->editor->clearDirty();
                }
                reply.json(glz::generic::object_t{ { "saved", DisplayPath(ResolvePath(*target->editor->filePath())) } });
            });
    }
} // namespace atmo::mcp::tools
