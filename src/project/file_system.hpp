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
#if defined(_WIN32)
    char *appdata = std::getenv("APPDATA");
    if (appdata) {
        return std::filesystem::path(appdata) / "ATMO" / "userdata";
    } else {
        throw std::runtime_error("APPDATA environment variable not set.");
    }
#elif defined(__APPLE__)
    char *home = std::getenv("HOME");
    if (home) {
        return std::filesystem::path(home) / "Library" / "Application Support" / "ATMO" / "userdata";
    } else {
        throw std::runtime_error("HOME environment variable not set.");
    }
#else
    char *home = std::getenv("HOME");
    if (home) {
        return std::filesystem::path(home) / ".local" / "share" / "ATMO" / "userdata";
    } else {
        throw std::runtime_error("HOME environment variable not set.");
    }
#endif
}

namespace atmo
{
    namespace project
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
                const char *path = nullptr;
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

            /**
             * @brief Opens a file from the given path.
             *
             * @param path Can be:
             *  - An absolute or relative path to a file on disk.
             *  - "project://relative/path/to/file" to open a file relative to the current project root or from the packed index.
             *  - "user://relative/path/to/file" to open a file relative to the user data directory.
             * @return File object representing the opened file.
             */
            static File OpenFile(std::string_view path, std::ios::openmode mode = std::ios::in | std::ios::out)
            {
                if (path.starts_with(PROJECT_PROTOCOL)) {
                    if (mode != (std::ios::in | std::ios::out))
                        spdlog::warn("Packed project files can only be opened in binary read mode.");

                    std::filesystem::path relative_path = std::string(path.substr(sizeof(PROJECT_PROTOCOL) - 1));
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
            static std::vector<std::string> SearchFiles(std::string_view path)
            {
                std::vector<std::string> results;

                if (path.starts_with(PROJECT_PROTOCOL)) {
                    std::filesystem::path relative_path = std::string(path.substr(sizeof(PROJECT_PROTOCOL) - 1));

                    for (const auto &pair : Instance().m_index) {
                        if (common::Utils::GlobMatch(relative_path.string(), pair.first)) {
                            results.push_back(pair.first);
                        }
                    }

                    return results;
                }

                if (path.starts_with(USER_PROTOCOL)) {
                    std::filesystem::path relative_path = std::string(path.substr(sizeof(USER_PROTOCOL) - 1));
                    std::filesystem::path full_path = GetUserDataDirectory() / relative_path;

                    for (const auto &entry : std::filesystem::directory_iterator(full_path)) {
                        results.push_back(entry.path().string());
                    }

                    return results;
                }

                std::filesystem::path full_path = std::string(path);
                for (const auto &entry : std::filesystem::directory_iterator(full_path)) {
                    results.push_back(entry.path().string());
                }
                return results;
            }

        private:
            FileSystem() = default;
            ~FileSystem()
            {
                if (m_resources && m_resources->is_open())
                    m_resources->close();

                for (auto &pair : m_index) {
                    std::free(const_cast<char *>(pair.second.path));
                }
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
                    entry.path = strdup(path.c_str());
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
        };
    } // namespace project
} // namespace atmo
