#pragma once

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <ios>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "common/utils.hpp"
#include "file.hpp"
#include "project_settings.hpp"
#include "spdlog/spdlog.h"

#define PROJECT_PROTOCOL "project://"
#define USER_PROTOCOL "user://"

#if defined(_WIN32)
#include <windows.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#else
#include <unistd.h>
#endif

namespace atmo::project
{
    class FileSystem
    {
    public:
#pragma pack(push, 1)
        typedef struct PackedHeader {
            char magic[8] = { ATMO_PACKED_MAGIC_NUMBER };
            uint32_t version{ 1 };
            uint32_t file_count{ 0 };
            uint64_t offset_to_files{ 0 };
        } PackedHeader;
#pragma pack(pop)

#pragma pack(push, 1)
        typedef struct PackedEntry {
            std::string path;
            uint64_t offset = 0;
            uint64_t size = 0;
        } PackedEntry;
#pragma pack(pop)

#if !defined(ATMO_EXPORT)
        static void DisplayPackedFileInfo(const std::filesystem::path &packed_file_path)
        {
            auto stream = std::make_shared<std::fstream>(packed_file_path, std::ios::binary | std::ios::in);
            std::unordered_map<std::string, PackedEntry> index;

            if (!stream->is_open()) {
                throw File::FileOpenException(packed_file_path.string());
            }

            FileSystem::PackedHeader header = LoadPackedIndex(stream, index);

            spdlog::info("Packed File: {}", packed_file_path.string());
            spdlog::info("Version: {}", header.version);
            spdlog::info("Number of files: {}", header.file_count);
            spdlog::info("Files:");

            for (const auto &pair : index) {
                const auto &entry = pair.second;
                spdlog::info(" - {} (Offset: {}, Size: {} bytes)", entry.path, entry.offset, entry.size);
            }
        }
#endif

#if !defined(ATMO_EXPORT)
        static void DisplayPackedFileContent(const std::filesystem::path &packed_file_path, const std::string &name)
        {
            auto stream = std::make_shared<std::fstream>(packed_file_path, std::ios::binary | std::ios::in);
            std::unordered_map<std::string, PackedEntry> index;

            if (!stream->is_open()) {
                throw File::FileOpenException(packed_file_path.string());
            }

            FileSystem::PackedHeader header = LoadPackedIndex(stream, index);

            if (!index.contains(name))
                throw std::runtime_error("File not found within packed file system.");

            stream->clear();
            stream->seekg(header.offset_to_files + index[name].offset, std::ios::beg);

            std::string buffer(index[name].size, '\0');
            stream->read(buffer.data(), index[name].size);

            std::cout.write(buffer.data(), stream->gcount());

            stream->clear();
            stream->seekg(0, std::ios::beg);
        }
#endif

        static std::filesystem::path GetRootPath()
        {
            return Instance().m_root;
        }

#if !defined(ATMO_EXPORT)
        /**
         * @brief Sets (or clears with an empty path) a real on-disk project root directory to consult
         *        for "project://" lookups, checked BEFORE the packed engine-asset index. Lets run mode
         *        (--project X --run) resolve a project's own assets/scenes straight from disk without
         *        packing them, while code that references the engine's own baked-in "project://assets/..."
         *        (fonts, icons) still falls through to the packed index unchanged. Not compiled into
         *        ATMO_EXPORT builds, since real export binaries are fully packed already.
         */
        static void SetProjectRootOverride(const std::filesystem::path &root)
        {
            Instance().m_project_root_override = root;
        }
#endif

        static void UpdateProjectName(const std::string &project_name)
        {
            Instance().m_project_name = project_name;
        }

        static std::string_view GetProjectName()
        {
            return Instance().m_project_name;
        }

        /**
         * @brief Resolves a path to a real, absolute filesystem location.
         *
         * @param path Can be:
         *  - An absolute or relative path on disk, returned as-is.
         *  - "user://relative/path" to resolve to a path relative to the user data directory.
         * @return The resolved filesystem path.
         */
        static std::filesystem::path ResolvePath(std::string_view path)
        {
            if (path.starts_with(USER_PROTOCOL))
                return GetUserDataDirectory() / std::string(path.substr(sizeof(USER_PROTOCOL) - 1));

            return std::filesystem::path(path);
        }

        /**
         * @brief Opens a file from the given path.
         *
         * @param path Can be:
         *  - An absolute or relative path to a file on disk.
         *  - "project://relative/path/to/file" to open a file from the packed index.
         *  - "user://relative/path/to/file" to open a file relative to the user data directory.
         * @param mode Mode used to open file as bitmask.
         * @return File object representing the opened file.
         * @exception File::FileOpenException If file could not be opened.
         * @exception std::runtime_error If file was not found.
         * @todo replace std::runtime_error when file is not found by FileSystem::FileNotFound exception.
         */
        static File OpenFile(std::string_view path, std::ios::openmode mode = std::ios::in | std::ios::out)
        {
            if (path.starts_with(PROJECT_PROTOCOL)) {
                std::filesystem::path relative_path = std::string(path.substr(sizeof(PROJECT_PROTOCOL) - 1));

#if !defined(ATMO_EXPORT)
                if (!Instance().m_project_root_override.empty()) {
                    std::filesystem::path disk_path = Instance().m_project_root_override / relative_path;
                    if (std::filesystem::exists(disk_path) && std::filesystem::is_regular_file(disk_path))
                        return File(disk_path.string(), mode);
                }
#endif

                if (mode != (std::ios::in | std::ios::out) || (mode & std::ios::in) == 0 && (mode & std::ios::binary) == 0)
                    spdlog::warn("Packed project files can only be opened in binary read mode.");

                auto it = Instance().m_index.find(relative_path.string());

                if (it != Instance().m_index.end()) {
                    const auto &entry = it->second;
                    std::uint64_t offset = Instance().m_header.offset_to_files + entry.offset;
                    return File(Instance().m_resources, offset, offset + entry.size);
                } else {
                    throw std::runtime_error(std::format(R"(File not found in packed file system: "{}")", relative_path.string()));
                }
            }

            if (path.starts_with(USER_PROTOCOL)) {
                std::filesystem::path relative_path = std::string(path.substr(sizeof(USER_PROTOCOL) - 1));
                return File((GetUserDataDirectory() / relative_path).string(), mode);
            }

            return File(path, mode);
        }

        /**
         * @brief Lists all files at the given path in the file system. Supports glob patterns.
         *
         * @param path Can be:
         *  - An absolute or relative path to a directory on disk.
         *  - "project://relative/path/to/directory" to list files relative to the current project root or from the packed index.
         *  - "user://relative/path/to/directory" to list files relative to the user data directory.
         * @return std::vector<std::string>
         */
        [[nodiscard]] static std::vector<std::string> SearchFiles(std::string_view path)
        {
            std::vector<std::string> results;

            if (path.starts_with(PROJECT_PROTOCOL)) {
                std::filesystem::path relative_path = std::string(path.substr(sizeof(PROJECT_PROTOCOL) - 1));
                std::unordered_set<std::string> seen;

#if !defined(ATMO_EXPORT)
                const auto &root_override = Instance().m_project_root_override;
                if (!root_override.empty()) {
                    if (ContainsGlob(relative_path.string())) {
                        std::filesystem::path base_dir = root_override / GlobBaseDir(relative_path);
                        if (std::filesystem::exists(base_dir)) {
                            for (const auto &entry : std::filesystem::recursive_directory_iterator(base_dir)) {
                                if (!entry.is_regular_file())
                                    continue;
                                auto rel = std::filesystem::relative(entry.path(), root_override).generic_string();
                                if (common::Utils::GlobMatch(relative_path.string(), rel) && seen.insert(rel).second)
                                    results.push_back(rel);
                            }
                        }
                    } else {
                        std::filesystem::path full_path = root_override / relative_path;
                        if (std::filesystem::is_directory(full_path)) {
                            for (const auto &entry : std::filesystem::directory_iterator(full_path)) {
                                auto rel = std::filesystem::relative(entry.path(), root_override).generic_string();
                                if (seen.insert(rel).second)
                                    results.push_back(rel);
                            }
                        } else if (std::filesystem::exists(full_path) && seen.insert(relative_path.generic_string()).second) {
                            results.push_back(relative_path.generic_string());
                        }
                    }
                }
#endif

                for (const auto &pair : Instance().m_index) {
                    if (common::Utils::GlobMatch(relative_path.string(), pair.first) && seen.insert(pair.first).second) {
                        results.push_back(pair.first);
                    }
                }

                return results;
            }

            if (path.starts_with(USER_PROTOCOL)) {
                std::filesystem::path relative_path = std::string(path.substr(sizeof(USER_PROTOCOL) - 1));
                std::filesystem::path user_dir = GetUserDataDirectory();

                if (ContainsGlob(relative_path.string())) {
                    std::filesystem::path base_dir = user_dir / GlobBaseDir(relative_path);
                    if (!std::filesystem::exists(base_dir))
                        return results;
                    for (const auto &entry : std::filesystem::recursive_directory_iterator(base_dir)) {
                        if (!entry.is_regular_file())
                            continue;
                        auto rel = std::filesystem::relative(entry.path(), user_dir);
                        if (common::Utils::GlobMatch(relative_path.string(), rel.string()))
                            results.push_back(entry.path().string());
                    }
                } else {
                    std::filesystem::path full_path = user_dir / relative_path;
                    for (const auto &entry : std::filesystem::directory_iterator(full_path)) results.push_back(entry.path().string());
                }

                return results;
            }

            std::filesystem::path full_path = std::string(path);
            if (ContainsGlob(full_path.string())) {
                std::filesystem::path base_dir = GlobBaseDir(full_path);
                if (!std::filesystem::exists(base_dir))
                    return results;
                for (const auto &entry : std::filesystem::recursive_directory_iterator(base_dir)) {
                    if (!entry.is_regular_file())
                        continue;
                    if (common::Utils::GlobMatch(full_path.string(), entry.path().string()))
                        results.push_back(entry.path().string());
                }
            } else {
                for (const auto &entry : std::filesystem::directory_iterator(full_path)) results.push_back(entry.path().string());
            }
            return results;
        }

    private:
        static bool ContainsGlob(std::string_view s)
        {
            return s.find_first_of("*?") != std::string_view::npos;
        }

        static std::filesystem::path GlobBaseDir(const std::filesystem::path &pattern)
        {
            std::string s = pattern.string();
            auto glob_pos = s.find_first_of("*?");
            if (glob_pos == std::string::npos)
                return pattern;
            auto slash_pos = s.rfind('/', glob_pos);
            if (slash_pos == std::string::npos)
                return std::filesystem::path(".");
            return std::filesystem::path(s.substr(0, slash_pos));
        }

        static std::filesystem::path GetExecutablePath()
        {
#if defined(_WIN32)
            std::vector<char> buffer(MAX_PATH);
            while (true) {
                DWORD size = GetModuleFileNameA(NULL, buffer.data(), static_cast<DWORD>(buffer.size()));
                if (size == 0)
                    return "";
                if (size < buffer.size()) {
                    buffer.resize(size);
                    break;
                }
                buffer.resize(buffer.size() * 2);
            }
            return std::filesystem::path(buffer.begin(), buffer.end());
#elif defined(__APPLE__)
            uint32_t size = 0;
            _NSGetExecutablePath(nullptr, &size);
            std::vector<char> buffer(size);
            if (_NSGetExecutablePath(buffer.data(), &size) == 0) {
                return std::filesystem::canonical(buffer.data());
            }
            return "";
#else // Linux / POSIX
            std::vector<char> buffer(1024);
            while (true) {
                ssize_t len = readlink("/proc/self/exe", buffer.data(), buffer.size());
                if (len == -1)
                    return "";
                if (static_cast<size_t>(len) < buffer.size()) {
                    buffer[len] = '\0';
                    return std::filesystem::canonical(buffer.data());
                }
                buffer.resize(buffer.size() * 2);
            }
#endif
        }

        static std::filesystem::path GetUserDataDirectory()
        {
#if !defined(ATMO_EXPORT)
#if defined(_WIN32)
            char *appdata = std::getenv("APPDATA");
            if (appdata) {
                return std::filesystem::path(appdata) / "atmo" / "userdata";
            } else {
                throw std::runtime_error("APPDATA environment variable not set.");
            }
#elif defined(__APPLE__)
            char *home = std::getenv("HOME");
            if (home) {
                return std::filesystem::path(home) / "Library" / "Application Support" / "atmo" / "userdata";
            } else {
                throw std::runtime_error("HOME environment variable not set.");
            }
#else
            char *home = std::getenv("HOME");
            if (home) {
                return std::filesystem::path(home) / ".local" / "share" / "atmo" / "userdata";
            } else {
                throw std::runtime_error("HOME environment variable not set.");
            }
#endif
#else
#if defined(_WIN32)
            char *appdata = std::getenv("APPDATA");
            if (appdata) {
                return std::filesystem::path(appdata) / atmo::project::FileSystem::GetProjectName();
            } else {
                throw std::runtime_error("APPDATA environment variable not set.");
            }
#elif defined(__APPLE__)
            char *home = std::getenv("HOME");
            if (home) {
                return std::filesystem::path(home) / "Library" / "Application Support" / atmo::project::FileSystem::GetProjectName();
            } else {
                throw std::runtime_error("HOME environment variable not set.");
            }
#else
            char *home = std::getenv("HOME");
            if (home) {
                return std::filesystem::path(home) / ".local" / "share" / atmo::project::FileSystem::GetProjectName();
            } else {
                throw std::runtime_error("HOME environment variable not set.");
            }
#endif
#endif
        }

        FileSystem() = default;
        ~FileSystem()
        {
            if (m_resources && m_resources->is_open())
                m_resources->close();
        }

        static std::vector<std::uint32_t> FindAllAtmoPcks(std::shared_ptr<std::fstream> stream)
        {
            static constexpr char ATMO_MAGIC_ARRAY[] = { ATMO_PACKED_MAGIC_NUMBER };
            static constexpr std::string_view magic{ ATMO_MAGIC_ARRAY, sizeof(ATMO_MAGIC_ARRAY) };
            static constexpr std::uint32_t BUF = 8 * 1024 * 1024;
            const std::uint32_t overlap = magic.size() - 1;
            std::vector<std::uint32_t> results;

            std::vector<char> buffer(BUF + overlap);

            size_t fileOffset = 0;

            while (stream) {
                stream->read(buffer.data() + overlap, BUF);
                size_t n = stream->gcount();
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

            stream->clear();
            stream->seekg(0, std::ios::beg);

            return results;
        }

        /**
         * @brief Find packed header in the binary.
         *
         * @param stream Stream to search.
         * @return Found packed header.
         */
        static FileSystem::PackedHeader FindPackedHeader(std::shared_ptr<std::fstream> stream)
        {
            static constexpr FileSystem::PackedHeader cmp_header = { 0 };
            auto positions = FindAllAtmoPcks(stream);
            stream->seekg(0, std::ios::end);
            std::uint32_t file_size = static_cast<std::uint32_t>(stream->tellg());
            stream->clear();

            for (const auto &pos : positions) {
                stream->seekg(pos, std::ios::beg);
                FileSystem::PackedHeader header;
                stream->read(reinterpret_cast<char *>(&header), sizeof(FileSystem::PackedHeader));

                if (header.file_count > 0 && header.offset_to_files > sizeof(FileSystem::PackedHeader) && header.offset_to_files < (file_size - pos) &&
                    header.version == cmp_header.version) {
                    header.offset_to_files += pos;
                    return header;
                }
            }

            throw std::runtime_error("Packed file header not found.");
        }

        static FileSystem::PackedHeader LoadPackedIndex(std::shared_ptr<std::fstream> stream, std::unordered_map<std::string, PackedEntry> &index)
        {
            FileSystem::PackedHeader header = FindPackedHeader(stream);

            for (uint32_t i = 0; i < header.file_count; ++i) {
                FileSystem::PackedEntry entry;
                std::string path;
                while (true) {
                    char c;
                    stream->get(c);
                    if (c == '\0')
                        break;
                    path += c;
                }
                entry.path = path;
                stream->read(reinterpret_cast<char *>(&entry.offset), sizeof(FileSystem::PackedEntry::offset));
                stream->read(reinterpret_cast<char *>(&entry.size), sizeof(FileSystem::PackedEntry::size));
                index[path] = entry;
            }

            return header;
        }

        static FileSystem &Instance()
        {
            static FileSystem instance;

            if (instance.m_root.empty()) {
                instance.m_root = GetExecutablePath();
            }

            if (!instance.m_resources)
                instance.m_resources = std::make_shared<std::fstream>();

            if (!instance.m_resources->is_open()) {
                instance.m_resources->open(instance.m_root.string(), std::ios::binary | std::ios::in);
                if (!instance.m_resources->is_open()) {
                    throw std::runtime_error("Failed to open project file system.");
                }
            }

            if (!instance.m_loaded) {
                instance.m_header = LoadPackedIndex(instance.m_resources, instance.m_index);
                instance.m_loaded = true;
            }

            return instance;
        }

        std::filesystem::path m_root;

        bool m_loaded{ false };
        PackedHeader m_header = { 0 };
        std::unordered_map<std::string, PackedEntry> m_index;
        std::shared_ptr<std::fstream> m_resources;
        std::string m_project_name;
#if !defined(ATMO_EXPORT)
        std::filesystem::path m_project_root_override;
#endif
    };
} // namespace atmo::project
