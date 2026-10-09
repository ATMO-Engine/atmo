#include "atmo_format.hpp"
#include <algorithm>
#include <cstring>
#include "project/file.hpp"
#include "project/file_system.hpp"
#include "spdlog/spdlog.h"

namespace atmo::image::extension
{
    std::size_t AtmoFormat::frameCount() const
    {
        return m_img.frame_nb;
    }

    Frame AtmoFormat::blankFrame() const
    {
        Frame frame;

        frame.frame.assign(m_img.size_y, std::vector<atmo::core::types::Color>(m_img.size_x, atmo::core::types::Color::TRANSPARENT_COL));
        return frame;
    }

    void AtmoFormat::syncFrameCounts()
    {
        const auto count = frameCount();
        if (count == 0)
            m_currentFrame = 0;
        else if (m_currentFrame >= count)
            m_currentFrame = static_cast<std::uint8_t>(count - 1);
    }

    void AtmoFormat::reset(std::uint16_t x, std::uint16_t y)
    {
        m_img = {};
        m_img.size_x = x;
        m_img.size_y = y;
        m_currentLayer = 0;
        m_currentFrame = 0;

        addLayer("Layer 1");
        addFrame();
    }

    Frame *AtmoFormat::currentFrame()
    {
        if (m_currentLayer >= m_img.layer_list.size() || m_currentFrame >= m_img.layer_list[m_currentLayer].frame_list.size())
            return nullptr;

        return &m_img.layer_list[m_currentLayer].frame_list[m_currentFrame];
    }

    const Frame *AtmoFormat::currentFrame() const
    {
        return const_cast<AtmoFormat *>(this)->currentFrame();
    }

    void AtmoFormat::BlendOnto(Frame &dst, const Frame &top)
    {
        for (std::size_t y = 0; y < dst.frame.size() && y < top.frame.size(); y++) {
            for (std::size_t x = 0; x < dst.frame[y].size() && x < top.frame[y].size(); x++) {
                const auto &src = top.frame[y][x];
                auto &out = dst.frame[y][x];

                const float outA = src.a + out.a * (1.0f - src.a);
                if (outA <= 0.0f) {
                    out = atmo::core::types::Color::TRANSPARENT_COL;
                    continue;
                }
                out.r = (src.r * src.a + out.r * out.a * (1.0f - src.a)) / outA;
                out.g = (src.g * src.a + out.g * out.a * (1.0f - src.a)) / outA;
                out.b = (src.b * src.a + out.b * out.a * (1.0f - src.a)) / outA;
                out.a = outA;
            }
        }
    }

    Frame AtmoFormat::renderAll() const
    {
        Frame result = blankFrame();

        for (auto lay = m_img.layer_list.rbegin(); lay != m_img.layer_list.rend(); ++lay) {
            if (!lay->visible || m_currentFrame >= lay->frame_list.size())
                continue;
            BlendOnto(result, lay->frame_list[m_currentFrame]);
        }

        return result;
    }

    std::vector<Frame> AtmoFormat::renderLayers(const std::vector<std::string> &layerNames) const
    {
        std::vector<Frame> result(frameCount(), blankFrame());

        for (auto lay = m_img.layer_list.rbegin(); lay != m_img.layer_list.rend(); ++lay) {
            if (std::find(layerNames.begin(), layerNames.end(), lay->name) == layerNames.end())
                continue;
            for (std::size_t f = 0; f < result.size() && f < lay->frame_list.size(); f++)
                BlendOnto(result[f], lay->frame_list[f]);
        }

        return result;
    }

    void AtmoFormat::addLayer(const std::string &layerName)
    {
        if (m_img.layer_list.size() >= UINT8_MAX) {
            spdlog::warn("Layer limit reached, {} not added", layerName);
            return;
        }

        for (const auto &lay : m_img.layer_list) {
            if (lay.name == layerName) {
                spdlog::warn("{} already exist, nothing added", layerName);
                return;
            }
        }

        Layer newLayer;
        newLayer.name = layerName;

        newLayer.frame_list.assign(frameCount(), blankFrame());
        m_img.layer_list.push_back(newLayer);
        m_img.layer_nb++;
    }

    void AtmoFormat::renameLayer(const std::string &layerName, const std::string &newName)
    {
        for (const auto &lay : m_img.layer_list) {
            if (lay.name == newName) {
                spdlog::warn("{} already exist, rename ignored", newName);
                return;
            }
        }

        for (auto &lay : m_img.layer_list) {
            if (lay.name == layerName) {
                lay.name = newName;
                return;
            }
        }

        spdlog::warn("{} not found, rename ignored", layerName);
    }

    void AtmoFormat::removeLayer(const std::string &layerName)
    {
        auto it = std::find_if(m_img.layer_list.begin(), m_img.layer_list.end(), [&](const Layer &lay){
            return lay.name == layerName;
        });

        if (it == m_img.layer_list.end()) {
            spdlog::warn("{} not found, nothing suppresed", layerName);
            return;
        }

        const auto idx = static_cast<std::uint8_t>(it - m_img.layer_list.begin());

        m_img.layer_list.erase(it);
        m_img.layer_nb--;
        if (m_currentLayer > idx || (m_currentLayer == idx && m_currentLayer >= m_img.layer_list.size())) {
            if (m_currentLayer > 0) {
                m_currentLayer--;
            }
        }
    }

    void AtmoFormat::moveLayer(const std::string &layerName, std::uint8_t desiredPos)
    {
        auto it = std::find_if(m_img.layer_list.begin(), m_img.layer_list.end(), [&](const Layer &lay){
            return lay.name == layerName;
        });

        if (it == m_img.layer_list.end()) {
            spdlog::warn("{} not found, move ignored", layerName);
            return;
        }

        const auto from = static_cast<std::size_t>(it - m_img.layer_list.begin());
        const auto to = std::min<std::size_t>(desiredPos, m_img.layer_list.size() - 1);
        const std::string selected = m_img.layer_list[m_currentLayer].name;

        if (from < to)
            std::rotate(m_img.layer_list.begin() + from, m_img.layer_list.begin() + from + 1, m_img.layer_list.begin() + to + 1);
        else if (from > to)
            std::rotate(m_img.layer_list.begin() + to, m_img.layer_list.begin() + from, m_img.layer_list.begin() + from + 1);

        selectLayer(selected);
    }

    void AtmoFormat::selectLayer(const std::string &layerName)
    {
        for (std::size_t i = 0; i < m_img.layer_list.size(); i++) {
            if (m_img.layer_list[i].name == layerName) {
                m_currentLayer = static_cast<std::uint8_t>(i);
                return;
            }
        }

        spdlog::warn("{} not found, selection unchanged", layerName);
    }

    void AtmoFormat::setLayerVisible(const std::string &layerName, bool visible)
    {
        for (auto &lay : m_img.layer_list) {
            if (lay.name == layerName) {
                lay.visible = visible;
                return;
            }
        }

        spdlog::warn("{} not found, visibility unchanged", layerName);
    }

    bool AtmoFormat::isLayerVisible(const std::string &layerName) const
    {
        for (const auto &lay : m_img.layer_list) {
            if (lay.name == layerName)
                return lay.visible;
        }

        return false;
    }

    Frame AtmoFormat::getFrame()
    {
        if (m_currentLayer >= m_img.layer_list.size() || m_currentFrame >= frameCount()) {
            spdlog::warn("No frame selected");
            return {};
        }

        return m_img.layer_list[m_currentLayer].frame_list[m_currentFrame];
    }

    void AtmoFormat::addFrame()
    {
        if (frameCount() >= UINT8_MAX) {
            spdlog::warn("Frame limit reached, nothing added");
            return;
        }

        const Frame frame = blankFrame();
        for (auto &lay : m_img.layer_list)
            lay.frame_list.push_back(frame);

        m_img.frame_nb++;
        syncFrameCounts();
    }

    void AtmoFormat::setFrameNumber(std::uint8_t size)
    {
        const Frame frame = blankFrame();
        for (auto &lay : m_img.layer_list)
            lay.frame_list.resize(size, frame);

        m_img.frame_nb = size;
        syncFrameCounts();
    }

    void AtmoFormat::moveFrame(std::uint8_t idx, std::uint8_t newIdx)
    {
        const auto count = frameCount();
        if (idx >= count || newIdx >= count) {
            spdlog::warn("Frame {} or {} out of range ({} frames), move ignored", idx, newIdx, count);
            return;
        }

        for (auto &lay : m_img.layer_list) {
            auto begin = lay.frame_list.begin();
            if (idx < newIdx)
                std::rotate(begin + idx, begin + idx + 1, begin + newIdx + 1);
            else if (idx > newIdx)
                std::rotate(begin + newIdx, begin + idx, begin + idx + 1);
        }

        if (m_currentFrame == idx)
            m_currentFrame = newIdx;
        else if (idx < m_currentFrame && m_currentFrame <= newIdx)
            m_currentFrame--;
        else if (newIdx <= m_currentFrame && m_currentFrame < idx)
            m_currentFrame++;
    }

    void AtmoFormat::removeFrame(std::uint8_t idx)
    {
        if (idx >= frameCount()) {
            spdlog::warn("Frame {} out of range, nothing suppresed", idx);
            return;
        }

        for (auto &lay : m_img.layer_list)
            lay.frame_list.erase(lay.frame_list.begin() + idx);

        m_img.frame_nb--;
        if (m_currentFrame > idx)
            m_currentFrame--;
        syncFrameCounts();
    }

    void AtmoFormat::selectFrame(std::uint8_t idx)
    {
        if (idx >= frameCount()) {
            spdlog::warn("Frame {} out of range, selection unchanged", idx);
            return;
        }

        m_currentFrame = idx;
    }

    void AtmoFormat::resizeFrame(std::uint16_t x, std::uint16_t y)
    {
        for (auto &lay : m_img.layer_list) {
            for (auto &frame : lay.frame_list) {
                frame.frame.resize(y);
                for (auto &row : frame.frame)
                    row.resize(x, atmo::core::types::Color::TRANSPARENT_COL);
            }
        }

        m_img.size_x = x;
        m_img.size_y = y;
    }

    bool AtmoFormat::load(std::string_view path)
    {
        try {
            atmo::project::File file = atmo::project::FileSystem::OpenFile(path);
            const std::string buffer = file.readAll();
            Reader reader(buffer);

            char magic[sizeof(MAGIC)];
            reader.need(sizeof(magic));
            for (auto &c : magic)
                c = reader.pod<char>();
            if (std::memcmp(magic, MAGIC, sizeof(MAGIC)) != 0)
                throw std::runtime_error("Not an atmo image");

            auto version = reader.pod<std::uint32_t>();
            if (version != VERSION)
                throw std::runtime_error(std::format("unsupported version {}", version));

            File img;
            auto layerCount = reader.pod<std::uint32_t>();
            if (layerCount > UINT8_MAX)
                throw std::runtime_error("too many layers");

            auto frameCount = reader.pod<std::uint32_t>();
            if (frameCount > UINT8_MAX)
                throw std::runtime_error("too many frames");

            auto sizeX = reader.pod<std::uint32_t>();
            auto sizeY = reader.pod<std::uint32_t>();
            if (sizeX > MAX_FRAME_SIZE || sizeY > MAX_FRAME_SIZE)
                throw std::runtime_error("frame too large");

            for (std::uint32_t l = 0; l < layerCount; l++) {
                Layer layer;
                layer.name = reader.string();
                layer.visible = reader.pod<std::uint8_t>() != 0;

                for (std::uint32_t f = 0; f < frameCount; f++) {
                    Frame frame;
                    reader.need(std::size_t(sizeX) * sizeY * sizeof(atmo::core::types::Color));

                    frame.frame.resize(sizeY);
                    for (auto &row : frame.frame) {
                        row.reserve(sizeX);
                        for (std::uint32_t x = 0; x < sizeX; x++)
                            row.push_back(reader.pod<atmo::core::types::Color>());
                    }
                    layer.frame_list.push_back(std::move(frame));
                }
                img.layer_list.push_back(std::move(layer));
            }
            img.layer_nb = static_cast<std::uint8_t>(img.layer_list.size());
            img.frame_nb = static_cast<std::uint8_t>(frameCount);
            img.size_x = static_cast<std::uint16_t>(sizeX);
            img.size_y = static_cast<std::uint16_t>(sizeY);

            m_img = std::move(img);
            m_currentLayer = 0;
            m_currentFrame = 0;
            return true;
        } catch (const std::exception &e) {
            spdlog::error("Failed to load atmo image \"{}\": {}", path, e.what());
            return false;
        }
    }

    bool AtmoFormat::save(std::string_view path) const
    {
        Writer writer;

        for (char c : MAGIC)
            writer.pod(c);
        writer.pod(VERSION);
        writer.pod(static_cast<std::uint32_t>(m_img.layer_list.size()));
        writer.pod(static_cast<std::uint32_t>(m_img.frame_nb));
        writer.pod(static_cast<std::uint32_t>(m_img.size_x));
        writer.pod(static_cast<std::uint32_t>(m_img.size_y));

        for (const auto &layer : m_img.layer_list) {
            writer.string(layer.name);
            writer.pod(static_cast<std::uint8_t>(layer.visible));

            for (std::size_t f = 0; f < m_img.frame_nb; f++) {
                static const Frame emptyFrame;
                const Frame &frame = f < layer.frame_list.size() ? layer.frame_list[f] : emptyFrame;

                for (std::uint32_t y = 0; y < m_img.size_y; y++) {
                    for (std::uint32_t x = 0; x < m_img.size_x; x++) {
                        const bool inBounds = y < frame.frame.size() && x < frame.frame[y].size();
                        const auto &color = inBounds ? frame.frame[y][x] : atmo::core::types::Color::TRANSPARENT_COL;
                        writer.pod(color);
                    }
                }
            }
        }

        try {
            auto resolved = atmo::project::FileSystem::ResolvePath(path);
            atmo::project::File file(resolved.string(), std::ios::out | std::ios::binary | std::ios::trunc);
            file.write(writer.data().data(), writer.data().size());
            return true;
        } catch (const std::exception &e) {
            spdlog::error("Failed to save atmo image \"{}\": {}", path, e.what());
            return false;
        }
    }
} // namespace atmo::image::extension
