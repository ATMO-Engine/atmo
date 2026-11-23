#include <algorithm>
#include <array>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

#include "core/engine.hpp"
#include "editor/editor_manager.hpp"
#include "editor/editor_registry.hpp"
#include "locale/locale_manager.hpp"
#include "mcp/mcp_server.hpp"
#include "mcp/tools/helpers.hpp"
#include "mcp/tools/tools.hpp"

namespace atmo::mcp::tools
{
    namespace editor_args
    {
        struct EditorOpenArgs {
            std::optional<std::string> path;
            std::optional<std::string> type;
            std::optional<bool> create;
        };

        struct EditorTargetArgs {
            std::string target;
        };

        struct EditorSaveArgs {
            std::optional<std::string> target;
            std::optional<std::string> path;
        };

        struct EditorCloseArgs {
            std::string target;
            std::optional<std::string> if_dirty;
        };

        struct CommandRunArgs {
            std::string id;
        };

        constexpr std::array blocked_commands{
            std::string_view("atmo.commands.file.save_as"),
            std::string_view("atmo.commands.file.open"),
            std::string_view("atmo.commands.file.quit"),
        };

        bool IsBlocked(std::string_view id)
        {
            return std::find(blocked_commands.begin(), blocked_commands.end(), id) != blocked_commands.end();
        }
    } // namespace editor_args
} // namespace atmo::mcp::tools

using namespace atmo::mcp::tools::editor_args;

template <> struct glz::json_schema<EditorOpenArgs> {
    schema path{ .description = "File to open; the editor type is picked from the extension (.ascn, .material, .atx)" };
    schema type{ .description = "Editor type (see editor_types); required when there is no path" };
    schema create{ .description = "Create the file when it does not exist" };
};
template <> struct glz::json_schema<EditorTargetArgs> {
    schema target{ .description = "Editor id or file path" };
};
template <> struct glz::json_schema<EditorSaveArgs> {
    schema target{ .description = "Editor id or file path (default: the focused tab)" };
    schema path{ .description = "Save to this path instead (save as)" };
};
template <> struct glz::json_schema<EditorCloseArgs> {
    schema target{ .description = "Editor id or file path" };
    schema if_dirty{ .description = "What to do with unsaved changes: 'error' (default), 'save' or 'discard'" };
};
template <> struct glz::json_schema<CommandRunArgs> {
    schema id{ .description = "Command id from command_list" };
};

namespace atmo::mcp::tools
{
    void RegisterEditorTools(McpServer &server)
    {
        auto &tools = server.tools();

        tools.add<NoArgs>("editor_types", "List the editor types and the file extensions they open.", [](const NoArgs &, const Reply &reply) {
            glz::generic::array_t types;
            for (const auto &entry : editor::EditorRegistry::GetEntries()) {
                if (editor::EditorRegistry::IsAbstract(entry))
                    continue;
                auto instance = editor::EditorRegistry::Create(entry);
                if (!instance)
                    continue;
                glz::generic::array_t extensions;
                for (auto extension : instance->fileExtensions()) extensions.emplace_back(std::string(extension));
                types.push_back(
                    glz::generic::object_t{
                        { "type", entry },
                        { "name", std::string(locale::LocaleManager::GetTranslation(std::string(instance->name()))) },
                        { "extensions", std::move(extensions) },
                    });
            }
            reply.json(glz::generic::object_t{ { "types", std::move(types) } });
        });

        tools.add<NoArgs>("editor_list", "List the open editor tabs.", [&server](const NoArgs &, const Reply &reply) {
            auto *manager = server.engine().editor();
            glz::generic::array_t editors;
            for (const auto &editor : manager->editors()) editors.push_back(EditorJson(*manager, *editor));
            reply.json(glz::generic::object_t{ { "editors", std::move(editors) } });
        });

        tools.add<EditorOpenArgs>(
            "editor_open",
            "Open an editor tab on a file (or focus the tab already open on it), or an empty editor of a type.",
            [&server](const EditorOpenArgs &args, const Reply &reply) {
                auto *manager = server.engine().editor();
                if (!args.path) {
                    if (!args.type)
                        return reply.error("Pass 'path' or 'type'");
                    auto editor = manager->openEditor(*args.type);
                    if (!editor)
                        return reply.error("Unknown editor type: " + *args.type + " (see editor_types)");
                    editor->setOpenedByMcp(true);
                    return reply.json(EditorJson(*manager, *editor));
                }

                const std::filesystem::path path = ResolvePath(*args.path);
                if (auto existing = manager->findEditorByPath(path)) {
                    manager->activateEditor(existing);
                    return reply.json(EditorJson(*manager, *existing));
                }

                std::optional<std::string> type = args.type ? args.type : editor::EditorManager::EditorTypeForPath(path);
                if (!type)
                    return reply.error("No editor opens " + path.extension().string() + " files (see editor_types)");

                std::shared_ptr<editor::Editor> editor;
                if (std::filesystem::exists(path)) {
                    editor = manager->openEditor(*type, path.string());
                } else {
                    if (!args.create.value_or(false))
                        return reply.error("File not found: " + *args.path + " (pass create: true to create it)");
                    std::filesystem::create_directories(path.parent_path());
                    editor = manager->openEditor(*type);
                    if (editor)
                        editor->saveAs(path.string());
                }
                if (!editor)
                    return reply.error("Could not open a " + *type);
                editor->setOpenedByMcp(true);
                reply.json(EditorJson(*manager, *editor));
            });

        tools.add<EditorTargetArgs>("editor_focus", "Switch to an editor tab.", [&server](const EditorTargetArgs &args, const Reply &reply) {
            auto *manager = server.engine().editor();
            auto editor = FindEditorTarget(*manager, args.target);
            if (!editor)
                return reply.error(editor.error());
            manager->activateEditor(*editor);
            reply.json(EditorJson(*manager, **editor));
        });

        tools.add<EditorSaveArgs>(
            "editor_save", "Save an editor tab to its file, or to a new path.", [&server](const EditorSaveArgs &args, const Reply &reply) {
                auto *manager = server.engine().editor();
                std::shared_ptr<editor::Editor> editor = manager->activeEditor();
                if (args.target) {
                    auto found = FindEditorTarget(*manager, *args.target);
                    if (!found)
                        return reply.error(found.error());
                    editor = *found;
                }
                if (!editor)
                    return reply.error("No editor tab is open");
                if (args.path) {
                    editor->saveAs(ResolvePath(*args.path).string());
                } else {
                    if (!editor->hasFilePath())
                        return reply.error("The tab has no file yet; pass 'path'");
                    editor->save();
                    editor->clearDirty();
                }
                reply.json(EditorJson(*manager, *editor));
            });

        tools.add<EditorCloseArgs>("editor_close", "Close an editor tab.", [&server](const EditorCloseArgs &args, const Reply &reply) {
            auto *manager = server.engine().editor();
            auto editor = FindEditorTarget(*manager, args.target);
            if (!editor)
                return reply.error(editor.error());
            const std::string policy = args.if_dirty.value_or("error");
            if ((*editor)->isDirty()) {
                if (policy == "save") {
                    if (!(*editor)->hasFilePath())
                        return reply.error("The tab has unsaved changes and no file; save it with editor_save and a path first");
                    (*editor)->save();
                } else if (policy != "discard") {
                    return reply.error("The tab has unsaved changes; pass if_dirty: 'save' or 'discard'");
                }
            }
            const std::uint32_t id = (*editor)->id();
            manager->closeEditor(*editor);
            reply.json(glz::generic::object_t{ { "closed", static_cast<double>(id) } });
        });

        tools.add<NoArgs>(
            "editor_close_auto", "Close the tabs that MCP tools opened and that have no unsaved changes.", [&server](const NoArgs &, const Reply &reply) {
                auto *manager = server.engine().editor();
                glz::generic::array_t closed;
                glz::generic::array_t kept;
                const auto editors = manager->editors();
                for (const auto &editor : editors) {
                    if (!editor->openedByMcp())
                        continue;
                    if (editor->isDirty()) {
                        kept.push_back(EditorJson(*manager, *editor));
                        continue;
                    }
                    closed.emplace_back(static_cast<double>(editor->id()));
                    manager->closeEditor(editor);
                }
                reply.json(glz::generic::object_t{ { "closed", std::move(closed) }, { "kept_dirty", std::move(kept) } });
            });

        tools.add<NoArgs>("command_list", "List the editor commands that command_run can execute.", [&server](const NoArgs &, const Reply &reply) {
            glz::generic::array_t commands;
            for (const auto &command : server.engine().editor()->commands().all()) {
                if (IsBlocked(command.id))
                    continue;
                commands.push_back(
                    glz::generic::object_t{
                        { "id", command.id },
                        { "category", std::string(locale::LocaleManager::GetTranslation(command.category)) },
                        { "label", std::string(locale::LocaleManager::GetTranslation(command.id)) },
                    });
            }
            reply.json(glz::generic::object_t{ { "commands", std::move(commands) } });
        });

        tools.add<CommandRunArgs>(
            "command_run", "Run an editor command, like the menu or its shortcut would.", [&server](const CommandRunArgs &args, const Reply &reply) {
                if (IsBlocked(args.id))
                    return reply.error("This command opens a dialog or quits; use editor_save with a path or editor_open instead");
                if (!server.engine().editor()->commands().execute(args.id))
                    return reply.error("Unknown command: " + args.id + " (see command_list)");
                reply.json(glz::generic::object_t{ { "ran", args.id } });
            });
    }
} // namespace atmo::mcp::tools
