#pragma once

#include <cerrno>
#include <cstring>
#include <filesystem>
#include <format>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <string>
#include "spdlog/spdlog.h"

namespace atmo
{
    namespace project
    {
        class File
        {
        public:
            explicit File(std::shared_ptr<std::fstream> base, std::uint64_t start, std::uint64_t end) :
                m_file(std::move(base)), m_start_offset(start), m_end_offset(end)
            {
            }

            explicit File(std::string_view path, std::ios::openmode mode = std::ios::in | std::ios::out) : m_start_offset(0), m_ownership(true)
            {
                m_file = std::make_shared<std::fstream>(path.data(), mode);

                if (!m_file->is_open())
                    throw FileOpenException(path.data());

                m_file->seekg(0, std::ios::end);
                m_end_offset = m_file->tellg();
                m_file->seekg(0, std::ios::beg);
            };

            ~File() = default;

            class FileOpenException : public std::runtime_error
            {
            public:
                explicit FileOpenException(const std::string &filename) :
                    m_reason(std::strerror(errno)), std::runtime_error(std::format("Failed to open file ({}): {}", m_reason, filename)), m_filename(filename)
                {
                }

                const std::string &getFilename() const
                {
                    return m_filename;
                }

                const std::string &getReason() const
                {
                    return m_reason;
                }

            private:
                std::string m_filename;
                std::string m_reason;
            };

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

            /**
             * @brief Reads the entire file content into a string.
             *
             * @return std::string The content of the file.
             */
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

            void write(const void *buf, std::uint64_t n)
            {
                if (!m_ownership || !m_file->good() || !(m_file->flags() & std::ios::out))
                    throw std::runtime_error("Cannot write to file.");

                m_file->seekp(m_start_offset + m_pos);
                m_file->write(reinterpret_cast<const char *>(buf), n);
                m_pos += n;
            }

        private:
            std::shared_ptr<std::fstream> m_file;
            bool m_ownership = false;
            uint64_t m_start_offset, m_end_offset = 0;
            uint64_t m_pos = 0;
        };
    } // namespace project
} // namespace atmo
