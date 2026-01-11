#pragma once

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <ios>
#include <string_view>
#include <unordered_map>

#include "project_settings.hpp"

#define PROJECT_PROTOCOL "project://"
#define USER_PROTOCOL "user://"

static inline std::filesystem::path get_user_data_directory()
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
        class File
        {
        public:
            explicit File(std::ifstream *base, std::uint64_t start, std::uint64_t end) : m_file(base), m_start_offset(start), m_end_offset(end) {}

            explicit File(std::string_view path) : m_start_offset(0), m_ownership(true)
            {
                m_file = new std::ifstream(path.data(), std::ios::binary | std::ios::ate);
                if (!m_file->is_open()) {
                    delete m_file;
                    throw std::runtime_error("Failed to open file: " + std::string(path));
                }
                m_end_offset = static_cast<std::uint64_t>(m_file->tellg());
                m_file->seekg(0, std::ios::beg);
            };

            ~File()
            {
                if (m_ownership && m_file) {
                    m_file->close();
                    delete m_file;
                }
            }

            /**
             * @brief Reads up to n bytes from the file into buf.
             *
             * @param buf Buffer to read data into.
             * @param n Maximum number of bytes to read.
             * @return std::size_t Number of bytes actually read.
             */
            std::size_t read(char *buf, std::size_t n)
            {
                std::uint64_t remaining = m_end_offset - m_start_offset - m_pos;
                std::size_t to_read = std::min<std::uint64_t>(n, remaining);
                m_file->seekg(m_start_offset + m_pos);
                m_file->read(buf, to_read);
                std::size_t bytes_read = m_file->gcount();
                m_pos += bytes_read;
                return bytes_read;
            }

            std::string readAll()
            {
                std::uint64_t size = m_end_offset - m_start_offset;
                std::string result(size, '\0');
                seek(0);
                read(result.data(), size);
                return result;
            }

            /**
             * @brief Seeks to a new position in the file.
             *
             * @param new_pos New position to seek to, relative to the start of the file segment.
             */
            void seek(uint64_t new_pos)
            {
                if (new_pos > m_end_offset - m_start_offset)
                    new_pos = m_end_offset - m_start_offset;
                m_pos = new_pos;
            }

        private:
            std::ifstream *m_file = nullptr;
            bool m_ownership = false;
            uint64_t m_start_offset, m_end_offset = 0;
            uint64_t m_pos = 0;
        };

        class FileSystem
        {
        public:
            typedef struct PackedHeader {
                char magic[4] = { 'A', 'T', 'M', 'O' };
                uint32_t version{ 1 };
                VERSION_TYPE major{ 0 }, minor{ 0 }, patch{ 0 };
                uint32_t file_count{ 0 };
                uint64_t offset_to_files{ 0 };
            } PackedHeader;

            typedef struct PackedEntry {
                const char *path = nullptr;
                uint64_t offset = 0;
                uint64_t size = 0;
            } PackedEntry;

            static void SetRootPath(std::filesystem::path path)
            {
                Instance().m_root = path.parent_path();
            }

            static std::filesystem::path GetRootPath()
            {
                return Instance().m_root;
            }

            /**
             * @brief Opens a file from the given path. If the file system is packed, it will open from the packed index.
             *
             * path can be:
             *  - An absolute path to a file on disk.
             *  - "project://relative/path/to/file" to open a file relative to the current project root or from the packed index.
             *  - "user://relative/path/to/file" to open a file relative to the user data directory.
             *
             * @param path
             * @return File object representing the opened file.
             */
            static File OpenFile(std::string_view path)
            {
                if (path.starts_with(PROJECT_PROTOCOL)) {
                    std::filesystem::path relative_path = std::string(path.substr(sizeof(PROJECT_PROTOCOL) - 1));
#if defined(ATMO_EXPORT)
                    auto it = Instance().m_index.find(relative_path.string());
                    if (it != Instance().m_index.end()) {
                        const auto &entry = it->second;
                        return File(Instance().resources, entry.offset, entry.offset + entry.size);
                    } else {
                        throw std::runtime_error("File not found in packed file system: " + relative_path.string());
                    }
#else
                    return File(Instance().m_root.string() + "/" + relative_path.string());
#endif
                }

                if (path.starts_with(USER_PROTOCOL)) {
                    std::filesystem::path relative_path = std::string(path.substr(sizeof(USER_PROTOCOL) - 1));
                    return File((get_user_data_directory() / relative_path).string());
                }

                return File(path);
            }

        private:
            FileSystem() = default;
            ~FileSystem()
            {
#if defined(ATMO_EXPORT)
                if (resources) {
                    resources->close();
                    delete resources;
                }
#endif
            }

            static FileSystem &Instance()
            {
                static FileSystem instance;
#if defined(ATMO_EXPORT)
                if (instance.resources == nullptr) {
                    instance.resources = new std::ifstream((instance.m_root).string(), std::ios::binary);
                    if (!instance.resources->is_open()) {
                        throw std::runtime_error("Failed to open packed resources file. (" + instance.m_root.string() + ")");
                    }
                }
#endif
                return instance;
            }
            std::filesystem::path m_root;

#if defined(ATMO_EXPORT)
            PackedHeader header = { 0 };
            std::unordered_map<std::string, PackedEntry> m_index;
            std::ifstream *resources = nullptr;
#else
#endif
        };
    } // namespace project
} // namespace atmo
