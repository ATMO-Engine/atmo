#pragma once

#include <chrono>
#include <cstring>
#include <filesystem>
#include <format>
#include <fstream>
#include <glaze/glaze.hpp>
#include <semver.hpp>
#include <set>
#include <spdlog/spdlog.h>
#include <string_view>
#include <system_error>
#include <vector>

#include "file_system.hpp"
#include "glaze/json/write.hpp"
#include "impl/romver.hpp"
#include "project/project_settings.hpp"

#define ATMO_PROJECT_SETTINGS_FILE "project_settings.json"
#define ATMO_PACKED_EXT ".pck"

namespace atmo
{
    namespace project
    {
        class ProjectManager
        {
        public:
#if !defined(ATMO_EXPORT)
            /**
             * @brief Opens a project from its root directory.
             *
             * Initializes the project environment by loading the project settings
             * defined in the project's project_settings.json file.
             *
             * @param project_path The filesystem path to the project's root directory.
             */
            inline static void OpenProject(const std::filesystem::path &project_path)
            {
                std::filesystem::path resolved_root = FileSystem::ResolvePath(project_path.string());
                std::filesystem::path settings_path = resolved_root / ".atmo" / ATMO_PROJECT_SETTINGS_FILE;

                if (!std::filesystem::exists(settings_path))
                    throw std::runtime_error("Invalid project path (no " ATMO_PROJECT_SETTINGS_FILE " found): " + settings_path.string());

                File settings_file = FileSystem::OpenFile(settings_path.string());
                LoadProjectSettings(settings_file, Instance().m_settings);

                Instance().m_project_root = resolved_root;
            }
#endif

#if !defined(ATMO_EXPORT)
            /**
             * @brief Creates a new project at the specified directory path.
             *
             * Creates a default project_settings.json file into the specified directory,
             * setting up the necessary structure and default settings for a new project.
             *
             * @param path The filesystem path where the new project_settings.json file will be created. Has to be a directory.
             *
             * @return The path to the created project_settings.json file.
             */
            static std::filesystem::path CreateProject(const std::filesystem::path &path)
            {
                std::filesystem::path resolved_path = FileSystem::ResolvePath(path.string());

                if (!std::filesystem::exists(resolved_path))
                    std::filesystem::create_directories(resolved_path / ".atmo");

                std::filesystem::path project_file_path = resolved_path / ".atmo" / ATMO_PROJECT_SETTINGS_FILE;
                if (std::filesystem::exists(project_file_path))
                    throw std::runtime_error("Project file already exists at: " + project_file_path.string());

                Instance().m_settings = {};
                Instance().m_settings.app.engine_version = impl::Romver::Parse(ATMO_VERSION);
                impl::Romver::Parse(ATMO_VERSION);

                std::ofstream project_file(project_file_path, std::ios::binary);
                if (!project_file.is_open())
                    throw std::runtime_error("Failed to create project file at: " + project_file_path.string());

                std::string dest;
                WriteProjectSettings(project_file, dest);
                project_file.write(dest.data(), dest.size());
                project_file.close();

                return project_file_path;
            }
#endif

            /**
             * @brief Get the Current Project Path object
             *
             * @return The filesystem path of the currently opened project.
             */
            inline static std::filesystem::path GetCurrentProjectPath()
            {
                return Instance().m_project_root;
            }

#if !defined(ATMO_EXPORT)
            /**
             * @brief Closes the currently opened project.
             *
             */
            inline static void CloseProject()
            {
                throw std::runtime_error("To be implemented.");
            }
#endif

#if !defined(ATMO_EXPORT)
            static std::set<std::filesystem::path> GetFilesToPack(const std::vector<std::string> &entries)
            {
                std::set<std::filesystem::path> out;

                for (const auto &entry : entries) {
                    if (!std::filesystem::exists(entry)) {
                        spdlog::warn(R"(File/Folder "{}" does not exist.)", entry);
                        continue;
                    } else if (std::filesystem::is_directory(entry)) {
                        for (const auto &rec_iter : std::filesystem::recursive_directory_iterator(entry)) {
                            if (rec_iter.is_regular_file())
                                out.emplace(rec_iter);
                        }
                    } else if (std::filesystem::is_regular_file(entry)) {
                        out.emplace(entry);
                    }
                }

                return out;
            }
#endif

#if !defined(ATMO_EXPORT)
            /**
             * @brief Generates a packed .pck file from the current project directory. This file may get appended to an atmo or atmo-export executable.
             *
             */
            static void GeneratePackedFile(std::string_view output_path = std::string_view(), const std::vector<std::string> &files = {})
            {
                std::string path = output_path.empty()
                    ? std::format("{}.{}.{}", Instance().m_settings.app.project_name, Instance().m_settings.app.project_version.toString(), ATMO_PACKED_EXT)
                    : std::string(output_path);
                std::ofstream out(path, std::ios::binary);
                if (!out.is_open())
                    throw std::runtime_error("Failed to create packed file.");

                FileSystem::PackedHeader header;

                std::set<std::filesystem::path> files_to_pack = GetFilesToPack(files);
                std::vector<FileSystem::PackedEntry> entries;

                for (const auto &entry : files_to_pack) {
                    std::ifstream in(entry, std::ios::binary | std::ios::ate);
                    if (!in.is_open()) {
                        spdlog::warn("Failed to open file for packing: {}", entry.string());
                        continue;
                    }
                    std::streamsize size = in.tellg();
                    in.seekg(0, std::ios::beg);
                    FileSystem::PackedEntry packed_entry;
                    packed_entry.path = entry.string();
                    packed_entry.offset = 0;
                    packed_entry.size = static_cast<std::uint64_t>(size);
                    entries.push_back(packed_entry);
                    in.close();
                }

                std::uint64_t current_offset = 0;
                for (auto &entry : entries) {
                    entry.offset = current_offset;
                    current_offset += entry.size;
                }

                header.file_count = static_cast<uint32_t>(entries.size());
                header.offset_to_files =
                    sizeof(FileSystem::PackedHeader) + (sizeof(FileSystem::PackedEntry::offset) + sizeof(FileSystem::PackedEntry::size)) * entries.size();
                for (const auto &entry : entries) {
                    header.offset_to_files += entry.path.size() + 1;
                }

                WriteStructure(out, &header);
                for (const auto &entry : entries) {
                    out.write(entry.path.c_str(), entry.path.size());
                    out.write("\0", 1);
                    out.write(reinterpret_cast<const char *>(&entry.offset), sizeof(std::uint64_t));
                    out.write(reinterpret_cast<const char *>(&entry.size), sizeof(std::uint64_t));
                }
                for (const auto &entry : entries) {
                    std::ifstream in(entry.path, std::ios::binary);
                    if (!in.is_open()) {
                        spdlog::warn("Failed to open file for packing data: {}", entry.path);
                        continue;
                    }
                    if (entry.size > 0) {
                        out << in.rdbuf();
                        if (!out.good()) {
                            spdlog::error("Write error while packing: {}", entry.path);
                            out.clear();
                        }
                    }
                    in.close();
                }

                out.close();
            };
#endif

#if !defined(ATMO_EXPORT)
            /**
             * @brief Bundles the currently open project's files into a .pck (reusing GetFilesToPack /
             *        GeneratePackedFile) and appends them onto a copy of a prebuilt atmo-export binary,
             *        producing a standalone distributable executable.
             *
             * @param export_binary_path Path to a prebuilt atmo-export binary (built via `xmake build atmo-export`).
             * @param output_path Path to write the resulting standalone executable to.
             * @return true on success.
             */
            static bool ExportProject(const std::filesystem::path &export_binary_path, const std::filesystem::path &output_path)
            {
                if (!std::filesystem::exists(export_binary_path)) {
                    spdlog::error("atmo-export binary not found at: {}", export_binary_path.string());
                    return false;
                }

                std::error_code ec;
                std::filesystem::copy_file(export_binary_path, output_path, std::filesystem::copy_options::overwrite_existing, ec);
                if (ec) {
                    spdlog::error("Failed to copy export binary to '{}': {}", output_path.string(), ec.message());
                    return false;
                }

                std::filesystem::path temp_pck = std::filesystem::temp_directory_path() /
                    std::format("atmo_export_{}{}", std::chrono::steady_clock::now().time_since_epoch().count(), ATMO_PACKED_EXT);

                // Pack from within the project root so entry keys come out project-root-relative
                // (e.g. "assets/icon.png"), matching the "project://" lookup convention — packing "."
                // instead would bake "./"-prefixed keys and silently break every lookup at runtime.
                std::filesystem::path saved_cwd = std::filesystem::current_path();
                bool ok = true;
                try {
                    std::filesystem::current_path(Instance().m_project_root);
                    std::vector<std::string> entries;
                    for (const auto &top : std::filesystem::directory_iterator(std::filesystem::current_path()))
                        entries.push_back(top.path().filename().string());
                    GeneratePackedFile(temp_pck.string(), entries);
                } catch (const std::exception &e) {
                    spdlog::error("Failed to pack project for export: {}", e.what());
                    ok = false;
                }
                std::filesystem::current_path(saved_cwd);
                if (!ok)
                    return false;

                {
                    std::ifstream src(temp_pck, std::ios::binary);
                    std::ofstream dst(output_path, std::ios::binary | std::ios::app);
                    if (!src.is_open() || !dst.is_open()) {
                        spdlog::error("Failed to append packed data to export binary.");
                        std::filesystem::remove(temp_pck, ec);
                        return false;
                    }
                    dst << src.rdbuf();
                }
                std::filesystem::remove(temp_pck, ec);

#if !defined(_WIN32)
                std::filesystem::permissions(output_path,
                    std::filesystem::perms::owner_exec | std::filesystem::perms::group_exec | std::filesystem::perms::others_exec,
                    std::filesystem::perm_options::add, ec);
                if (ec)
                    spdlog::warn("Failed to set executable permission on '{}': {}", output_path.string(), ec.message());
#endif

                spdlog::info("Exported project to '{}'", output_path.string());
                return true;
            }
#endif

            static ProjectSettings &GetSettings()
            {
                return Instance().m_settings;
            }

#if !defined(ATMO_EXPORT)
            /**
             * @brief Writes the current in-memory project settings back to the currently opened project's
             *        project_settings.json.
             */
            static void SaveSettings()
            {
                std::filesystem::path settings_path = Instance().m_project_root / ".atmo" / ATMO_PROJECT_SETTINGS_FILE;

                std::ofstream file(settings_path, std::ios::binary);
                if (!file.is_open())
                    throw std::runtime_error("Failed to open project settings file for writing: " + settings_path.string());

                std::string dest;
                WriteProjectSettings(file, dest);
                file.write(dest.data(), dest.size());
            }
#endif

        private:
            ProjectManager()
            {
                try {
                    auto settings_search = project::FileSystem::SearchFiles("project://.atmo/project_settings.json");

                    if (!settings_search.empty()) {
                        File settings_file = project::FileSystem::OpenFile("project://.atmo/project_settings.json");
                        LoadProjectSettings(settings_file, m_settings);
                        FileSystem::UpdateProjectName(m_settings.app.project_name);
                    } else {
                        FileSystem::UpdateProjectName("atmo");
                    }
                } catch (const std::runtime_error &err) {
                    spdlog::warn("Couldn't load project settings: {}", err.what());
                };
            }

            ~ProjectManager() = default;

            static ProjectManager &Instance()
            {
                static ProjectManager m_instance;
                return m_instance;
            }

            ProjectSettings m_settings;
            std::filesystem::path m_project_root;

            inline static void LoadProjectSettings(File &file, ProjectSettings &settings)
            {
                std::string content = file.readAll();
                auto err = glz::read_json<ProjectSettings>(settings, content);

                if (err) {
                    std::string descriptive_error = glz::format_error(err, content);
                    throw std::runtime_error("Failed to parse project settings: " + descriptive_error);
                }

                spdlog::debug("Loaded project settings for project: {}", settings.app.project_name);
            }

            inline static void WriteProjectSettings(std::ofstream &file, std::string &dest)
            {
                auto err = glz::write_json(Instance().m_settings, dest);

                if (err) {
                    std::string descriptive_error = glz::format_error(err, dest);
                    throw std::runtime_error("Failed to serialize project settings: " + descriptive_error);
                }
            }

            template <typename T> inline static void WriteStructure(std::ofstream &file, const T *setting)
            {
                file.write(reinterpret_cast<const char *>(setting), sizeof(T));
            }
        };
    } // namespace project
} // namespace atmo
