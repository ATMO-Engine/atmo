#pragma once

#include "editor/editor_registry.hpp"
#include "editor/editors/editor.hpp"
#include "editor/editors/scene_editor/editor_scene_context.hpp"

namespace atmo::editor
{
    class TextureEditor : public EditorRegistry::Registrable<TextureEditor, Editor>
    {
    public:
        using EditorRegistry::Registrable<TextureEditor, Editor>::Registrable;

        static constexpr std::string_view LocalName()
        {
            return "TextureEditor";
        }

        std::string_view name() override
        {
            return "atmo.editors.texture_editor.name";
        }

        std::string_view description() override
        {
            return "atmo.editors.texture_editor.description";
        }

        std::string_view iconPath() override
        {
            return "project://assets/icons/brush.svg";
        }

        std::string_view getTypeName() const override
        {
            return FullName();
        }

        void init(atmo::core::ecs::entities::UI &container) override;
        void createTools() override;

        void save() override;
        void load() override;

    private:
        void refreshTimeline();
        void refreshAllLayerVisible();

        /**
         * @brief Asks for the frame count and frame size, then slices the raster image into an .atmo image
         *
         * @param path Raster image to import
         * @param sheetSize Size of the whole image, used as the default frame size
         */
        void openSpriteSheetImportPopup(const std::string &path, core::types::Vector2i sheetSize);

        /**
         * @brief Asks for a file name and a format, then exports the image next to the opened file
         *
         * .atmo writes the image as is. A raster format (png, bmp, jpg) also asks which layers to blend and writes
         * every frame of the blend side by side in a sprite sheet.
         */
        void openExportPopup();

        /**
         * @brief Shows the frame step frames away from the current one and highlights it in the timeline
         *
         * @param step Frames to move by, negative to go back, wraps around the frame count
         */
        void stepFrame(int step);

        /**
         * @brief Starts or stops the animation, drawing on the canvas is locked while it plays
         */
        void setPlaying(bool playing);

        /**
         * @brief Moves the animation forward by as many frames as elapsed at m_fps, called every tick
         *
         * @param deltaTime Seconds since the last tick
         */
        void tickAnimation(float deltaTime);

        static constexpr int DEFAULT_FPS = 60;
        static constexpr int MAX_FPS = 240;

        int m_fps = DEFAULT_FPS;
        bool m_playing = false;
        float m_frame_time_accumulator = 0.0f;

        bool m_allLayerVisible = true;

        flecs::entity m_root_handle;
        flecs::entity m_toggle_all_label_handle;
        std::vector<flecs::entity> m_visibility_label_handles;
        flecs::entity m_canvas_handle;
        flecs::entity m_timeline_handle;
        flecs::entity m_viewport_image;
        std::unique_ptr<EditorSceneContext> m_scene_ctx;
        std::vector<std::function<void()>> m_inspector_update_fns;
    };
} // namespace atmo::editor
