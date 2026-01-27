#pragma once

#include <cstring>
#include <filesystem>
#include <fstream>
#include <glaze/glaze.hpp>
#include <semver.hpp>
#include <spdlog/spdlog.h>
#include <string_view>
#include <vector>

#include "file_system.hpp"
#include "glaze/json/write.hpp"
#include "project/project_settings.hpp"

#define ATMO_PROJECT_FILE "project.atmo"
#define ATMO_PACKED_EXT ".pck"

namespace atmo
{
    namespace project
    {
        class ProjectManager
        {
        public:
            /**
             * @brief Opens a project from the path to a project.atmo file.
             *
             * Initializes the project environment by loading the project settings,
             * resources, and scenes defined in the project.atmo file.
             *
             * @param path The filesystem path to the project.atmo file.
             */
            inline static void OpenProject(const std::filesystem::path &path)
            {
#if defined(ATMO_EXPORT)
                throw std::runtime_error("Cannot open project of an exported application.");
#else
                if (!std::filesystem::exists(path) || path.filename() != ATMO_PROJECT_FILE)
                    throw std::runtime_error("Invalid project file path: " + path.string());

                FileSystem::SetRootPath(path);
                File project_file = FileSystem::OpenFile(".atmo/" + std::string(ATMO_PROJECT_FILE));
                LoadProjectSettings(project_file);

                semver::version<VERSION_TYPES> current_engine_version;
                semver::parse(ATMO_VERSION_STRING, current_engine_version);

                if (Instance().m_settings.app.engine_version < current_engine_version) {
                    spdlog::warn(
                        "Project engine version ({}) is older than the current engine version ({}). It will be overwritten on save.",
                        Instance().m_settings.app.engine_version.to_string(),
                        current_engine_version.to_string());
                }
#endif
            }

            /**
             * @brief Creates a new project at the specified directory path.
             *
             * Creates a default project.atmo file into the specified directory,
             * setting up the necessary structure and default settings for a new project.
             *
             * @param path The filesystem path where the new project.atmo file will be created. Has to be a directory.
             *
             * @return The path to the created project.atmo file.
             */
            static std::filesystem::path CreateProject(const std::filesystem::path &path)
            {
#if defined(ATMO_EXPORT)
                throw std::runtime_error("Cannot create project of an exported application.");
#else
                if (!std::filesystem::exists(path))
                    std::filesystem::create_directories(path / ".atmo");

                std::filesystem::path project_file_path = path / ".atmo" / ATMO_PROJECT_FILE;
                if (std::filesystem::exists(project_file_path))
                    throw std::runtime_error("Project file already exists at: " + project_file_path.string());

                Instance().m_settings = {};
                semver::parse(ATMO_VERSION_STRING, Instance().m_settings.app.engine_version);

                std::ofstream project_file(project_file_path, std::ios::binary);
                if (!project_file.is_open())
                    throw std::runtime_error("Failed to create project file at: " + project_file_path.string());

                std::string dest;
                WriteProjectSettings(project_file, dest);
                project_file.write(dest.data(), dest.size());
                project_file.close();

                return project_file_path;
#endif
            }

            /**
             * @brief Get the Current Project Path object
             *
             * @return The filesystem path of the currently opened project.
             */
            inline static std::filesystem::path GetCurrentProjectPath()
            {
                return FileSystem::GetRootPath();
            }

            /**
             * @brief Closes the currently opened project.
             *
             */
            inline static void CloseProject()
            {
#if defined(ATMO_EXPORT)
                throw std::runtime_error("Cannot close project of an exported application.");
#endif
            }

            /**
             * @brief Generates a packed .pck file from the current project directory. This file may get appended to an atmo or atmo-export executable.
             *
             */
            static void GeneratePackedFile(std::string_view output_path = std::string_view(), const std::vector<std::string> &files = {})
            {
#if defined(ATMO_EXPORT)
                throw std::runtime_error("Cannot generate packed file from an exported application.");
#else
                std::string path = output_path.empty() ? std::format(
                                                             "{}.{}.{}",
                                                             Instance().m_settings.app.project_name,
                                                             Instance().m_settings.app.project_version.to_string(),
                                                             std::string(ATMO_PACKED_EXT, 4))
                                                       : std::string(output_path);
                std::ofstream out(path, std::ios::binary);
                if (!out.is_open())
                    throw std::runtime_error("Failed to create packed file.");

                FileSystem::PackedHeader header;

                std::vector<FileSystem::PackedEntry> entries;

                if (files.empty()) {
                    for (const auto &entry : std::filesystem::recursive_directory_iterator(GetCurrentProjectPath())) {
                        if (entry.is_regular_file() && entry.path().filename() != ATMO_PROJECT_FILE) {
                            std::ifstream in(entry.path(), std::ios::binary | std::ios::ate);
                            if (!in.is_open()) {
                                spdlog::warn("Failed to open file for packing: {}", entry.path().string());
                                continue;
                            }
                            std::streamsize size = in.tellg();
                            in.seekg(0, std::ios::beg);
                            FileSystem::PackedEntry packed_entry;
                            packed_entry.path = strdup(entry.path().lexically_relative(GetCurrentProjectPath()).string().c_str());
                            packed_entry.offset = 0;
                            packed_entry.size = static_cast<uint64_t>(size);
                            entries.push_back(packed_entry);
                            in.close();
                        }
                    }
                } else {
                    for (const auto &entry : files) {
                        std::ifstream in(entry, std::ios::binary | std::ios::ate);
                        if (!in.is_open()) {
                            spdlog::warn("Failed to open file for packing: {}", entry);
                            continue;
                        }
                        std::streamsize size = in.tellg();
                        in.seekg(0, std::ios::beg);
                        FileSystem::PackedEntry packed_entry;
                        packed_entry.path = entry.c_str();
                        packed_entry.offset = 0;
                        packed_entry.size = static_cast<uint64_t>(size);
                        entries.push_back(packed_entry);
                        in.close();
                    }
                }

                uint64_t current_offset = sizeof(FileSystem::PackedHeader);
                for (auto &entry : entries) {
                    entry.offset = current_offset;
                    current_offset += entry.size;
                }

                header.file_count = static_cast<uint32_t>(entries.size());
                header.offset_to_files = sizeof(FileSystem::PackedHeader) + sizeof(FileSystem::PackedEntry) * entries.size();

                WriteStructure(out, &header);
                for (const auto &entry : entries) {
                    out.write(entry.path, std::strlen(entry.path) + 1);
                    out.write(reinterpret_cast<const char *>(&entry.offset), sizeof(uint64_t));
                    out.write(reinterpret_cast<const char *>(&entry.size), sizeof(uint64_t));
                }
                for (const auto &entry : entries) {
                    std::ifstream in(entry.path, std::ios::binary);
                    if (!in.is_open()) {
                        spdlog::warn("Failed to open file for packing data: {}", entry.path);
                        continue;
                    }
                    out << in.rdbuf();
                    in.close();
                }

                out.close();
#endif
            };

            static void DisplayPackedFileInfo(const std::filesystem::path &packed_file_path)
            {
#if defined(ATMO_EXPORT)
                throw std::runtime_error("Cannot display packed file info from an exported application.");
#else
                std::ifstream in(packed_file_path, std::ios::binary);
                if (!in.is_open())
                    throw std::runtime_error("Failed to open packed file: " + packed_file_path.string());

                FileSystem::PackedHeader header;

                FindPackedHeader(in, &header);

                spdlog::info("Packed file info for: {}", packed_file_path.string());
                spdlog::info(" - Packed Files: {} files", header.file_count);
                for (uint32_t i = 0; i < header.file_count; ++i) {
                    FileSystem::PackedEntry entry;
                    std::string path;
                    while (true) {
                        char c;
                        in.get(c);
                        if (c == '\0')
                            break;
                        path += c;
                    }
                    entry.path = strdup(path.c_str());
                    in.read(reinterpret_cast<char *>(&entry.offset), sizeof(uint64_t));
                    in.read(reinterpret_cast<char *>(&entry.size), sizeof(uint64_t));
                    spdlog::info("   - {} (Size: {} bytes, Offset: {})", entry.path, entry.size, entry.offset);
                }

                in.close();
#endif
            };

            static ProjectSettings &GetSettings()
            {
                return Instance().m_settings;
            }

        private:
            ProjectManager() = default;
            ~ProjectManager() = default;

            static ProjectManager &Instance()
            {
                static ProjectManager m_instance;
                return m_instance;
            }

            ProjectSettings m_settings;

            inline static void LoadProjectSettings(File &file)
            {
                std::string content = file.readAll();
                auto err = glz::read_json<ProjectSettings>(Instance().m_settings, content);

                if (err) {
                    std::string descriptive_error = glz::format_error(err, content);
                    throw std::runtime_error("Failed to parse project settings: " + descriptive_error);
                }

                spdlog::debug("Loaded project settings for project: {}", Instance().m_settings.app.project_name);
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

            static std::vector<std::uint32_t> FindAllAtmoPcks(std::ifstream &in)
            {
                static constexpr char ATMO_MAGIC_ARRAY[] = { ATMO_PACKED_MAGIC_NUMBER };
                static constexpr std::string_view magic{ ATMO_MAGIC_ARRAY, sizeof(ATMO_MAGIC_ARRAY) };
                constexpr std::uint32_t BUF = 8 * 1024 * 1024;
                const std::uint32_t overlap = magic.size() - 1;
                std::vector<std::uint32_t> results;

                std::vector<char> buffer(BUF + overlap);

                size_t fileOffset = 0;

                while (in) {
                    in.read(buffer.data() + overlap, BUF);
                    size_t n = in.gcount();
                    if (!n)
                        break;

                    const char *begin = buffer.data();
                    const char *end = begin + overlap + n;

                    auto searcher = std::boyer_moore_horspool_searcher(magic.begin(), magic.end());

                    auto it = std::search(begin, end, searcher);
                    while (it != end) {
                        size_t pos = fileOffset + (it - begin) - overlap;
                        results.push_back(pos);

                        it = std::search(it + 1, end, searcher);
                    }

                    std::copy(end - overlap, end, buffer.data());

                    fileOffset += n;
                }

                return results;
            }

            static void FindPackedHeader(std::ifstream &in, FileSystem::PackedHeader *out_header)
            {
                auto positions = FindAllAtmoPcks(in);
                in.seekg(0, std::ios::end);
                std::uint32_t file_size = static_cast<std::uint32_t>(in.tellg());
                in.clear();

                for (const auto &pos : positions) {
                    in.seekg(pos, std::ios::beg);
                    FileSystem::PackedHeader header;
                    in.read(reinterpret_cast<char *>(&header), sizeof(FileSystem::PackedHeader));

                    if (header.file_count > 0 && header.offset_to_files > sizeof(FileSystem::PackedHeader) && header.offset_to_files < (file_size - pos) &&
                        header.version == out_header->version) {
                        *out_header = header;
                        return;
                    }
                }

                throw std::runtime_error("Packed file header not found.");
            }
        };
    } // namespace project
} // namespace atmo
