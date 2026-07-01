#pragma once

#include <memory>
#include <string>

#include "SDL3/SDL_render.h"
#include "core/types.hpp"
#include "flecs.h"

namespace atmo::core::ecs::entities
{
    class Camera2d;
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
         * @brief Initialize the isolated world and off-screen render texture.
         *        Must be called before tick(). WorldContext singleton is set before RegisterSystems,
         *        so physics systems are skipped automatically.
         *
         * @param renderer  SDL_Renderer borrowed from the main Window (non-owning).
         */
        void init(SDL_Renderer *renderer);

        /**
         * @brief Advance the isolated world by delta_time.
         *        @p renderer is the current, validated SDL_Renderer from the main Window.
         *        The caller must verify the renderer is still alive before calling tick().
         */
        void tick(float delta_time, SDL_Renderer *renderer);

        /**
         * @brief Resize the off-screen texture. Does nothing if size is unchanged.
         */
        void resize(int width, int height);

        /**
         * @brief Returns the off-screen SDL_Texture that receives the isolated world's rendering.
         *        The caller must NOT destroy this texture.
         */
        SDL_Texture *getViewportTexture() const
        {
            return m_render_texture;
        }

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
        void createRenderTexture(int width, int height);
        void destroyRenderTexture();
        void drawOverlays();

        flecs::world m_world;
        SDL_Texture *m_render_texture = nullptr;
        SDL_Renderer *m_renderer = nullptr;
        std::shared_ptr<core::ecs::entities::Scene> m_scene;
        std::shared_ptr<core::ecs::entities::Camera2d> m_camera;
        bool m_ready = false;
        int m_width = 0;
        int m_height = 0;
    };
} // namespace atmo::editor
