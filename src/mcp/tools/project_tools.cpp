#include <filesystem>
#include <fstream>
#include <map>
#include <optional>
#include <string>
#include <variant>

#include "core/engine.hpp"
#include "core/resource/material.hpp"
#include "core/resource/resource_manager.hpp"
#include "core/resource/shader.hpp"
#include "core/types.hpp"
#include "editor/editor_manager.hpp"
#include "editor/editors/material_editor/material_editor.hpp"
#include "mcp/mcp_server.hpp"
#include "mcp/tools/helpers.hpp"
#include "mcp/tools/tools.hpp"
#include "project/project_manager.hpp"
#include "project/project_settings.hpp"
#include "server/rendering/rendering_server.hpp"

namespace atmo::mcp::tools
{
    namespace project_args
    {
        namespace rendering = server::rendering;

        struct SettingsGetArgs {
            std::optional<std::string> section;
        };

        struct SettingsSetArgs {
            std::string section;
            glz::generic value;
        };

        struct AssetTreeArgs {
            std::optional<std::string> dir;
            std::optional<std::string> extension;
            std::optional<int> limit;
        };

        struct PathArgs {
            std::string path;
        };

        struct MaterialSetArgs {
            std::string path;
            std::optional<std::string> shader;
            std::optional<std::map<std::string, glz::generic>> params;
            std::optional<bool> save;
        };

        glz::generic SettingsJson()
        {
            auto json = glz::write_json(project::ProjectManager::GetSettings());
            if (!json)
                return glz::generic::object_t{};
            return glz::read_json<glz::generic>(*json).value_or(glz::generic::object_t{});
        }

        glz::generic ParamJson(const rendering::ShaderParam &param)
        {
            glz::generic::object_t json{ { "name", param.name }, { "type", std::string(rendering::ShaderParamTypeName(param.type)) } };
            if (param.range)
                json["range"] = glz::generic::array_t{ static_cast<double>((*param.range)[0]), static_cast<double>((*param.range)[1]) };
            return json;
        }

        glz::generic ValueJson(const core::resource::MaterialValue &value)
        {
            return std::visit(
                [](const auto &v) -> glz::generic {
                    using T = std::decay_t<decltype(v)>;
                    if constexpr (std::is_same_v<T, double>) {
                        return v;
                    } else if constexpr (std::is_same_v<T, core::resource::MaterialTextureRef>) {
                        return glz::generic::object_t{
                            { "path", v.path },
                            { "filter", v.filter == rendering::TextureFilter::Nearest ? "nearest" : "linear" },
                            { "repeat", v.repeat },
                        };
                    } else {
                        glz::generic::array_t array;
                        for (float component : v) array.emplace_back(static_cast<double>(component));
                        return array;
                    }
                },
                value);
        }

        template <std::size_t N> std::expected<std::array<float, N>, std::string> FloatArray(const glz::generic &json)
        {
            std::array<float, N> out{};
            if (!json.is_array() || json.get_array().size() != N)
                return std::unexpected(std::format("expected an array of {} numbers", N));
            for (std::size_t i = 0; i < N; ++i) {
                if (!json.get_array()[i].is_number())
                    return std::unexpected(std::format("expected an array of {} numbers", N));
                out[i] = static_cast<float>(json.get_array()[i].get_number());
            }
            return out;
        }

        std::expected<core::resource::MaterialValue, std::string> ParseValue(rendering::ShaderParamType type, const glz::generic &json)
        {
            using rendering::ShaderParamType;
            switch (type) {
                case ShaderParamType::Int:
                case ShaderParamType::Uint:
                case ShaderParamType::Float:
                    if (!json.is_number())
                        return std::unexpected("expected a number");
                    return json.get_number();
                case ShaderParamType::Float2:
                    return FloatArray<2>(json);
                case ShaderParamType::Float3:
                    return FloatArray<3>(json);
                case ShaderParamType::Color:
                    if (json.is_string()) {
                        const core::types::Color color(json.get_string());
                        return std::array<float, 4>{ color.r, color.g, color.b, color.a };
                    }
                    return FloatArray<4>(json);
                case ShaderParamType::Float4:
                    return FloatArray<4>(json);
                case ShaderParamType::Texture:
                    {
                        core::resource::MaterialTextureRef ref;
                        if (json.is_string()) {
                            ref.path = json.get_string();
                            return ref;
                        }
                        if (!json.is_object() || !json.contains("path") || !json["path"].is_string())
                            return std::unexpected("expected a texture path or { path, filter, repeat }");
                        ref.path = json["path"].get_string();
                        if (json.contains("filter") && json["filter"].is_string())
                            ref.filter = json["filter"].get_string() == "nearest" ? rendering::TextureFilter::Nearest : rendering::TextureFilter::Linear;
                        if (json.contains("repeat") && json["repeat"].is_boolean())
                            ref.repeat = json["repeat"].get_boolean();
                        return ref;
                    }
            }
            return std::unexpected("unsupported parameter type");
        }

        std::shared_ptr<editor::MaterialEditor> MaterialEditorOn(McpServer &server, const std::filesystem::path &path)
        {
            auto *manager = server.engine().editor();
            if (!manager)
                return nullptr;
            return std::dynamic_pointer_cast<editor::MaterialEditor>(manager->findEditorByPath(path));
        }

        std::string MaterialKey(McpServer &server, const std::filesystem::path &path)
        {
            if (auto editor = MaterialEditorOn(server, path); editor && editor->hasFilePath())
                return *editor->filePath();
            return path.string();
        }

        glz::generic MaterialJson(const std::filesystem::path &path, core::resource::Material &material)
        {
            glz::generic::object_t json{ { "path", DisplayPath(path) }, { "shader", material.shaderPath() } };
            glz::generic::object_t values;
            for (const auto &[name, value] : material.values()) values[name] = ValueJson(value);

            if (const auto &shader = material.shader()) {
                json["kind"] = std::string(rendering::ShaderKindName(shader->kind()));
                glz::generic::array_t params;
                for (const auto &param : shader->layout().params) {
                    glz::generic entry = ParamJson(param);
                    if (auto value = material.get(param.name))
                        entry["value"] = ValueJson(*value);
                    else
                        entry["value"] = ValueJson(core::resource::Material::DefaultValue(param));
                    values.erase(param.name);
                    params.push_back(std::move(entry));
                }
                json["params"] = std::move(params);
            } else {
                json["shader_error"] = "The shader could not be loaded (see shader_status)";
            }
            if (!values.empty())
                json["unused_values"] = std::move(values);
            return json;
        }
    } // namespace project_args
} // namespace atmo::mcp::tools

using namespace atmo::mcp::tools::project_args;

template <> struct glz::json_schema<SettingsGetArgs> {
    schema section{ .description = "Settings section (app, boot, window, rendering, singletons, engine, debug, addons, mcp); default: all" };
};
template <> struct glz::json_schema<SettingsSetArgs> {
    schema section{ .description = "Settings section" };
    schema value{ .description = "Field values merged into the section (JSON merge patch)" };
};
template <> struct glz::json_schema<AssetTreeArgs> {
    schema dir{ .description = "Directory to list (default: the project root)" };
    schema extension{ .description = "Only list files with this extension, e.g. '.ascn'" };
    schema limit{ .description = "Maximum number of entries (default 1000)" };
};
template <> struct glz::json_schema<PathArgs> {
    schema path{ .description = "File path" };
};
template <> struct glz::json_schema<MaterialSetArgs> {
    schema path{ .description = ".material file; created when it does not exist and 'shader' is given" };
    schema shader{ .description = ".slang shader file of the material" };
    schema params{ .description = "Parameter values by name: numbers, arrays of 2-4 numbers, '#rrggbb' or [r, g, b, a] colors, texture paths" };
    schema save{ .description = "Write the .material file (default true)" };
};

namespace atmo::mcp::tools
{
    void RegisterProjectTools(McpServer &server)
    {
        auto &tools = server.tools();

        tools.add<NoArgs>("project_info", "Describe the open project and the engine instance.", [&server](const NoArgs &, const Reply &reply) {
            const auto &settings = project::ProjectManager::GetSettings();
            reply.json(
                glz::generic::object_t{
                    { "name", settings.app.project_name },
                    { "path", project::ProjectManager::GetCurrentProjectPath().generic_string() },
                    { "default_scene", settings.app.default_scene },
                    { "atmo_version", ATMO_VERSION },
                    { "mode", server.mode() == McpServer::Mode::Editor ? "editor" : "game" },
                    { "headless", server.engine().isHeadless() },
                    { "rendering_platform", std::string(rendering::PlatformKindName(rendering::RenderingServer::Instance().platformKind())) },
                    { "frame", static_cast<double>(server.engine().frameIndex()) },
                });
        });

        tools.add<SettingsGetArgs>("project_settings_get", "Read the project settings.", [](const SettingsGetArgs &args, const Reply &reply) {
            glz::generic json = SettingsJson();
            if (!args.section)
                return reply.json(json);
            if (!json.contains(*args.section))
                return reply.error("Unknown section: " + *args.section);
            reply.json(json[*args.section]);
        });

        tools.add<SettingsSetArgs>(
            "project_settings_set",
            "Change project settings and save them. Most settings apply the next time the project is opened.",
            [](const SettingsSetArgs &args, const Reply &reply) {
                glz::generic json = SettingsJson();
                if (!json.contains(args.section))
                    return reply.error("Unknown section: " + args.section);
                json[args.section] = MergePatch(json[args.section], args.value);

                project::ProjectSettings updated = project::ProjectManager::GetSettings();
                const std::string text = json.dump().value_or("{}");
                if (auto ec = glz::read_json(updated, text))
                    return reply.error("Invalid settings: " + glz::format_error(ec, text));
                project::ProjectManager::GetSettings() = updated;
                project::ProjectManager::SaveSettings();
                reply.json(SettingsJson()[args.section]);
            });

        tools.add<AssetTreeArgs>("asset_tree", "List the files of the project.", [](const AssetTreeArgs &args, const Reply &reply) {
            const std::filesystem::path root = ResolvePath(args.dir.value_or("."));
            if (!std::filesystem::is_directory(root))
                return reply.error("Not a directory: " + args.dir.value_or("."));
            const std::size_t limit = static_cast<std::size_t>(std::max(1, args.limit.value_or(1000)));

            glz::generic::array_t files;
            bool truncated = false;
            std::error_code ec;
            for (auto it = std::filesystem::recursive_directory_iterator(root, std::filesystem::directory_options::skip_permission_denied, ec);
                 it != std::filesystem::recursive_directory_iterator();
                 it.increment(ec)) {
                if (ec)
                    break;
                const std::string name = it->path().filename().string();
                if (name.starts_with('.')) {
                    if (it->is_directory())
                        it.disable_recursion_pending();
                    continue;
                }
                if (it->is_directory())
                    continue;
                if (args.extension && it->path().extension() != *args.extension)
                    continue;
                if (files.size() >= limit) {
                    truncated = true;
                    break;
                }
                files.emplace_back(DisplayPath(it->path()));
            }
            reply.json(glz::generic::object_t{ { "files", std::move(files) }, { "truncated", truncated } });
        });

        tools.add<PathArgs>(
            "material_get", "Read a material: its shader, parameters (type, range, value).", [&server](const PathArgs &args, const Reply &reply) {
                const std::filesystem::path path = ResolvePath(args.path);
                if (!std::filesystem::exists(path))
                    return reply.error("Material not found: " + args.path);
                auto material = core::resource::ResourceManager::GetInstance().getResource<core::resource::Material>(MaterialKey(server, path));
                if (!material)
                    return reply.error("Could not load " + args.path + " (see logs_read)");
                reply.json(MaterialJson(path, *material));
            });

        tools.add<MaterialSetArgs>(
            "material_set", "Change a material's shader and parameter values, and save it.", [&server](const MaterialSetArgs &args, const Reply &reply) {
                const std::filesystem::path path = ResolvePath(args.path);
                if (!std::filesystem::exists(path)) {
                    if (!args.shader)
                        return reply.error("Material not found: " + args.path + " (pass 'shader' to create it)");
                    std::filesystem::create_directories(path.parent_path());
                    std::ofstream(path) << R"({ "shader": "", "params": {} })";
                }

                auto material = core::resource::ResourceManager::GetInstance().getResource<core::resource::Material>(MaterialKey(server, path));
                if (!material)
                    return reply.error("Could not load " + args.path + " (see logs_read)");

                if (args.shader && !material->setShader(*args.shader))
                    return reply.error("Could not load the shader " + *args.shader + " (see shader_status)");

                if (args.params) {
                    for (const auto &[name, json] : *args.params) {
                        const rendering::ShaderParam *param = material->shader() ? material->shader()->layout().find(name) : nullptr;
                        if (!param)
                            return reply.error("The shader has no parameter '" + name + "' (see material_get)");
                        auto value = ParseValue(param->type, json);
                        if (!value)
                            return reply.error("Parameter '" + name + "': " + value.error());
                        if (!material->set(name, *value))
                            return reply.error("Could not set parameter '" + name + "'");
                    }
                }

                if (args.save.value_or(true) && !material->save(path.string()))
                    return reply.error("Could not write " + args.path);

                if (auto editor = MaterialEditorOn(server, path))
                    editor->reloadFromMaterial();
                reply.json(MaterialJson(path, *material));
            });

        tools.add<PathArgs>(
            "shader_status",
            "Compile a .slang material shader and report its errors, or its kind and parameters.",
            [](const PathArgs &args, const Reply &reply) {
                const std::filesystem::path path = ResolvePath(args.path);
                if (!std::filesystem::exists(path))
                    return reply.error("Shader not found: " + args.path);
                auto desc = core::resource::Shader::CompileFile(path.string());
                if (!desc)
                    return reply.json(glz::generic::object_t{ { "ok", false }, { "error", desc.error() } });
                glz::generic::array_t params;
                for (const auto &param : desc->layout.params) params.push_back(ParamJson(param));
                reply.json(
                    glz::generic::object_t{
                        { "ok", true },
                        { "kind", std::string(rendering::ShaderKindName(desc->kind)) },
                        { "params", std::move(params) },
                    });
            });
    }
} // namespace atmo::mcp::tools
