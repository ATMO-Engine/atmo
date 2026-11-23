#pragma once

#include <expected>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

#include "core/ecs/entities/entity.hpp"
#include "core/ecs/entities/scene/scene.hpp"
#include "flecs.h"
#include "glaze/glaze.hpp"
#include "meta/type_info.hpp"
#include "server/rendering/types.hpp"

namespace atmo::editor
{
    class Editor;
    class EditorManager;
    class SceneEditor;
} // namespace atmo::editor

namespace atmo::mcp
{
    class McpServer;
}

namespace atmo::mcp::tools
{
    struct SceneTarget {
        flecs::world *world = nullptr;
        std::shared_ptr<core::ecs::entities::Scene> scene;
        std::shared_ptr<editor::SceneEditor> editor;
    };

    struct ComponentRef {
        const meta::TypeInfo *type = nullptr;
        flecs::id_t id = 0;
    };

    [[nodiscard]] editor::EditorManager *EditorManagerOf(McpServer &server);
    [[nodiscard]] std::expected<editor::EditorManager *, std::string> RequireEditor(McpServer &server);

    [[nodiscard]] std::filesystem::path ResolvePath(const std::string &path);
    [[nodiscard]] std::string DisplayPath(const std::filesystem::path &path);

    [[nodiscard]] std::expected<std::shared_ptr<editor::Editor>, std::string> FindEditorTarget(editor::EditorManager &manager, const std::string &target);
    [[nodiscard]] glz::generic EditorJson(editor::EditorManager &manager, const editor::Editor &editor);

    [[nodiscard]] std::expected<SceneTarget, std::string> ResolveScene(McpServer &server, const std::optional<std::string> &scene);
    [[nodiscard]] std::expected<core::ecs::entities::Entity, std::string> ResolveEntity(const SceneTarget &target, std::string_view reference);
    void MarkDirty(const SceneTarget &target);

    [[nodiscard]] std::string EntityPath(const SceneTarget &target, const core::ecs::entities::Entity &entity);
    [[nodiscard]] std::string EntityType(const core::ecs::entities::Entity &entity);
    [[nodiscard]] glz::generic EntitySummary(const SceneTarget &target, const core::ecs::entities::Entity &entity);
    [[nodiscard]] glz::generic::object_t ComponentsJson(flecs::entity entity);
    [[nodiscard]] glz::generic::array_t ComponentNames(flecs::entity entity);
    [[nodiscard]] std::optional<ComponentRef> FindComponent(flecs::world &world, std::string_view name);
    [[nodiscard]] std::expected<void, std::string> SetComponent(flecs::entity entity, std::string_view name, const glz::generic &value);
    [[nodiscard]] std::optional<std::string> ResolveEntityType(std::string_view type);

    [[nodiscard]] glz::generic MergePatch(const glz::generic &target, const glz::generic &patch);
    [[nodiscard]] std::optional<std::string> EncodePngBase64(const server::rendering::TextureData &data);
    [[nodiscard]] std::string Base64(std::string_view bytes);
} // namespace atmo::mcp::tools
