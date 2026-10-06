#pragma once

#include <array>
#include <bit>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>
#include "core/types.hpp"

namespace atmo::image::extension
{

/**  .atmo binary layout (little-endian, as written by the host):

char[8]   magic "ATMOIMG\0"
uint32    version
uint32    layer count
uint32    frame count (shared by every layer)
uint32    size_x
uint32    size_y
per layer:
    uint32    name length, then name bytes (no '\0')
    uint8     visible
    per frame:
        size_x * size_y * Color pixels (4 float32: r, g, b, a), row by row (frame[y][x])

*/

    struct Frame {
        std::vector<std::vector<atmo::core::types::Color>> frame;
    };

    struct Layer {
        std::string name;
        bool visible = true;

        std::vector<Frame> frame_list;
    };

    struct File {
        std::uint8_t layer_nb = 0;
        std::uint8_t frame_nb = 0;
        std::uint16_t size_x = 0;
        std::uint16_t size_y = 0;

        std::vector<Layer> layer_list;
    };

    class AtmoFormat {
        public:
            /**
             * @brief Replaces the image with a blank one made of a single layer holding a single frame
             *
             * @param x Width of the frames
             * @param y Height of the frames
             */
            void reset(std::uint16_t x, std::uint16_t y);

            /**
             * @brief Gets the selected frame of the selected layer, for in place edition
             *
             * @return nullptr if there is no layer or no frame
             */
            Frame *currentFrame();
            const Frame *currentFrame() const;

            std::uint16_t width() const { return m_img.size_x; }
            std::uint16_t height() const { return m_img.size_y; }

            std::vector<std::string> layerNames() const
            {
                std::vector<std::string> names;
                for (const auto &lay : m_img.layer_list)
                    names.push_back(lay.name);
                return names;
            }
            std::uint8_t currentLayer() const { return m_currentLayer; }

            /**
             * @brief Blends the current frame of every visible layer into a single frame
             *
             * The last layer of the list is drawn first and the first layer ends up on top of all the others.
             *
             * @return A size_x * size_y frame
             */
            Frame renderAll() const;

            void addLayer(const std::string &layerName);
            void renameLayer(const std::string &layerName, const std::string &newName);
            void removeLayer(const std::string &layerName);
            void moveLayer(const std::string &layerName, std::uint8_t desiredPos);
            void selectLayer(const std::string &layerName);
            void setLayerVisible(const std::string &layerName, bool visible);
            bool isLayerVisible(const std::string &layerName) const;

            Frame getFrame();
            void addFrame();
            void setFrameNumber(std::uint8_t size);
            void moveFrame(std::uint8_t idx, std::uint8_t newIdx);
            void removeFrame(std::uint8_t idx);
            void selectFrame(std::uint8_t idx);
            void resizeFrame(std::uint16_t x, std::uint16_t y);

            /**
             * @brief Loads an .atmo image into m_img
             *
             * @param path Disk path, "user://..." or "project://...".
             * @return false if the file could not be opened or is malformed (m_img is left untouched).
             */
            bool load(std::string_view path);

            /**
             * @brief Saves m_img to an .atmo image, overwriting the file if it exists
             *
             * @param path Disk path, "user://..." or "project://...".
             * @return false if the file could not be written.
             */
            bool save(std::string_view path) const;
        private:
            std::size_t frameCount() const;
            Frame blankFrame() const;
            void syncFrameCounts();

            File m_img;
            std::uint8_t m_currentLayer = 0;
            std::uint8_t m_currentFrame = 0;
    };


    constexpr char MAGIC[8] = { 'A', 'T', 'M', 'O', 'I', 'M', 'G', '\0' };
    constexpr std::uint32_t VERSION = 1;

    static_assert(sizeof(atmo::core::types::Color) == 4 * sizeof(float), "Color layout changed, update the .atmo format and bump VERSION");

    class Writer {
        public:
            /**
             * @brief Appends the raw bytes of a value to the buffer
             *
             * Only works for "plain old data" types (integers, floats, chars, simple structs like Color) that can be
             * copied byte by byte. Types owning memory (std::string, std::vector...) are rejected at compile time
             * since copying their bytes would only copy a pointer, not the content.
             *
             * @param value The value to append, it takes exactly sizeof(T) bytes
             */
            template <typename T> void pod(const T &value)
            {
                static_assert(std::is_trivially_copyable_v<T>);
                m_buffer.append(reinterpret_cast<const char *>(&value), sizeof(T));
            }

            /**
             * @brief Appends a string as its length (uint32) followed by its characters
             *
             * The length prefix lets the Reader know how many bytes to read back, no '\0' is written.
             *
             * @param str The string to append
             */
            void string(const std::string &str)
            {
                pod(static_cast<std::uint32_t>(str.size()));
                m_buffer.append(str);
            }

            /**
             * @brief Gets everything written so far
             *
             * @return The serialized bytes, ready to be written to a file
             */
            const std::string &data() const
            {
                return m_buffer;
            }

        private:
            std::string m_buffer;
    };

    class Reader {
        public:
            /**
             * @brief Creates a reader starting at the beginning of the buffer
             *
             * @param buffer The bytes to read from, usually the whole file content
             */
            explicit Reader(const std::string &buffer) : m_buffer(buffer) {}

            /**
             * @brief Reads the next sizeof(T) bytes as a T and moves the cursor past them
             *
             * Same restriction as Writer::pod, only plain old data types. The bytes are copied into a byte array
             * (the buffer isn't guaranteed to be aligned for T) then std::bit_cast turns them into a T, so T doesn't
             * need a default constructor (e.g. Color).
             *
             * @return The value read
             * @exception std::runtime_error If fewer than sizeof(T) bytes are left
             */
            template <typename T> T pod()
            {
                static_assert(std::is_trivially_copyable_v<T>);
                need(sizeof(T));
                std::array<char, sizeof(T)> bytes;
                std::memcpy(bytes.data(), m_buffer.data() + m_pos, sizeof(T));
                m_pos += sizeof(T);
                return std::bit_cast<T>(bytes);
            }

            /**
             * @brief Reads a string written by Writer::string (uint32 length then characters)
             *
             * @return The string read
             * @exception std::runtime_error If the buffer ends before the length or the characters
             */
            std::string string()
            {
                auto size = pod<std::uint32_t>();
                need(size);
                std::string str = m_buffer.substr(m_pos, size);
                m_pos += size;
                return str;
            }

            /**
             * @brief Checks that at least n bytes are left to read, without moving the cursor
             *
             * Called by pod and string, but also useful before a big loop (e.g. all the pixels of a frame) to reject
             * a bad file before allocating memory for data that isn't there.
             *
             * @param n Number of bytes that must be available
             * @exception std::runtime_error If fewer than n bytes are left
             */
            void need(std::size_t n) const
            {
                if (n > m_buffer.size() - m_pos)
                    throw std::runtime_error("unexpected end of file");
            }
        private:
            const std::string &m_buffer;
            std::size_t m_pos = 0;
    };
} // namespace atmo::image::extension
