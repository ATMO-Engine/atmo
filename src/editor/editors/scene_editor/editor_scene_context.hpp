#pragma once

#include <memory>
#include <string>

#include "core/types.hpp"
#include "flecs.h"
#include "server/rendering/rid.hpp"

namespace atmo::core::ecs::entities
{
    class Scene;
} // namespace atmo::core::ecs::entities

namespace atmo::editor
{
    class EditorSceneContext
    {
    public:
        EditorSceneContext() = default;
        ~EditorSceneContext();

        EditorSceneContext(const EditorSceneContext &) = delete;
        EditorSceneContext &operator=(const EditorSceneContext &) = delete;

        /**
         * @brief Initialize the isolated world and its offscreen viewport.
         *        Must be called before tick(). WorldContext singleton is set before RegisterSystems,
         *        so physics systems are skipped automatically.
         *
         * @param dpi_scale Pixels per logical unit of the window showing the viewport.
         */
        void init(core::types::Vector2 dpi_scale);

        /**
         * @brief Advance the isolated world by delta_time and rebuild the overlays.
         */
        void tick(float delta_time);

        /**
         * @brief Resize the viewport, in logical units. Does nothing if size is unchanged.
         */
        void resize(int width, int height);

        /**
         * @brief Sets the window's pixel scale; the viewport texture is rendered at size * dpi_scale pixels.
         */
        void setDpiScale(core::types::Vector2 dpi_scale);

        /**
         * @brief Returns the viewport's texture, stable for the lifetime of the context. Show it in a UIImage.
         */
        server::rendering::TextureRid getViewportTexture() const;

        /**
         * @brief Returns true after init() has succeeded.
         */
        bool isReady() const
        {
            return m_ready;
        }

        int getWidth() const
        {
            return m_width;
        }
        int getHeight() const
        {
            return m_height;
        }

        /**
         * @brief Serialize the isolated scene to a prettified JSON string.
         */
        std::string saveSceneToJson() const;

        /**
         * @brief Write the serialized scene JSON to a file path on disk.
         */
        void saveSceneToFile(const std::string &path) const;

        /**
         * @brief Load a scene from a JSON string into the isolated world.
         *        Destroys existing scene children first.
         */
        void loadSceneFromJson(const std::string &json);

        /**
         * @brief Load a scene from a file path on disk.
         */
        void loadSceneFromFile(const std::string &path);

        flecs::world &getWorld()
        {
            return m_world;
        }

        std::shared_ptr<core::ecs::entities::Scene> getScene() const
        {
            return m_scene;
        }

        /**
         * @brief Pan the viewport by @p delta_screen pixels (screen space).
         */
        void pan(core::types::Vector2 delta_screen);

        /**
         * @brief Zoom the viewport by @p factor toward @p pivot_screen (screen-space pivot point).
         */
        void zoom(float factor, core::types::Vector2 pivot_screen);

        /**
         * @brief Convert a screen-space point (relative to the render texture top-left) to world space.
         */
        core::types::Vector2 screenToWorld(core::types::Vector2 screen) const;

        /**
         * @brief Convert a world-space point to screen space (texture pixels).
         */
        core::types::Vector2 worldToScreen(core::types::Vector2 world) const;

    private:
        void applyViewportSize();
        void drawOverlays();

        flecs::world m_world;
        server::rendering::CanvasRid m_canvas;
        server::rendering::ViewportRid m_viewport;
        server::rendering::CanvasItemRid m_overlay;
        core::types::Vector2 m_dpi_scale{ 1.0f, 1.0f };
        std::shared_ptr<core::ecs::entities::Scene> m_scene;
        core::types::Vector2 m_cam_position{ 0.0f, 0.0f };
        float m_cam_zoom = 1.0f;
        bool m_ready = false;
        int m_width = 0;
        int m_height = 0;
    };
} // namespace atmo::editor
