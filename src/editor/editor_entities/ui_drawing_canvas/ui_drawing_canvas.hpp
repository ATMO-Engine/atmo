#pragma once

#include <exception>
#include <format>
#include <string>
#include <unordered_set>
#include "clay.h"
#include "core/ecs/entities/ui/ui.hpp"
#include "core/types.hpp"
#include "editor/editors/texture_editor/atmo_format.hpp"

namespace atmo::core::components
{

    struct UIDrawingCanvas {
        enum class ExportFormat {
            PNG,
            BMP,
            JPG
        };

        enum class DrawType {
            PENCIL,
            ERASER
        };

        Clay_BoundingBox bounds = {};
        SDL_Texture *display_texture = nullptr;
        core::types::Vector2i display_texture_size = { 0, 0 };

        atmo::image::extension::AtmoFormat image;
        std::vector<uint8_t> upload_buffer;
        uint32_t current_stroke_id = 0;
        std::unordered_set<uint32_t> painted_pixels;

        SDL_Texture *drawing_texture = nullptr;
        SDL_Texture *checkboard_texture = nullptr;
        bool texture_dirty = true;

        std::string file_path;
        ExportFormat format = ExportFormat::PNG;

        atmo::core::types::Vector2 canvas_size = { 0.0f, 0.0f };
        atmo::core::types::Vector2i texture_size = { 1, 1 };

        SDL_FRect cached_texture_rect = {};
        float zoom = 1.0f;
        atmo::core::types::Vector2 offset = { 0.0f, 0.0f };
        atmo::core::types::Vector2 last_paint_mouse_pos = { 0.0f, 0.0f };
        atmo::core::types::Vector2 last_pan_mouse_pos = { 0.0f, 0.0f };
        bool panning = false;

        int brush_radius = 10;
        atmo::core::types::Color brush_color = atmo::core::types::Color::BLACK;
        DrawType pen = DrawType::PENCIL;

        bool preview = false;
        bool drawing_locked = false; // Ignores brush strokes, zoom and pan still work (e.g. while the animation plays)
    };
} // namespace atmo::core::components

template <> struct atmo::meta::ComponentMeta<atmo::core::components::UIDrawingCanvas> {
    static constexpr const char *name = "DrawingCanvas";
    static constexpr const char *category = "UI";
    static constexpr auto fields = std::make_tuple(
        atmo::meta::field<&atmo::core::components::UIDrawingCanvas::zoom>("zoom"),
        atmo::meta::field<&atmo::core::components::UIDrawingCanvas::offset>("offset"),
        atmo::meta::field<&atmo::core::components::UIDrawingCanvas::brush_radius>("brush radius"));
};

namespace atmo::core::ecs::entities
{
    constexpr float PAN_MARGIN_FACTOR = 0.25f;
    constexpr int MAX_FRAME_SIZE = atmo::image::extension::MAX_FRAME_SIZE;
    constexpr int DEFAULT_TEXTURE_SIZE = 64;

    class UIDrawingCanvas : public EntityRegistry::Registrable<UIDrawingCanvas, UI>
    {
    public:
        using EntityRegistry::Registrable<UIDrawingCanvas, UI>::Registrable;

        /**
         * @brief Thrown when an image can't be imported into the canvas
         *
         * The canvas is left untouched, callers catch it and treat the import as a no op.
         */
        class ImportException : public std::exception
        {
        public:
            ImportException(const std::string &path, const std::string &reason) :
                m_path(path),
                m_reason(reason),
                m_message(std::format(R"(Failed to import "{}": {})", path, reason))
            {
            }

            const char *what() const noexcept override
            {
                return m_message.c_str();
            }

            const std::string &getPath() const
            {
                return m_path;
            }

            const std::string &getReason() const
            {
                return m_reason;
            }

        private:
            std::string m_path;
            std::string m_reason;
            std::string m_message;
        };

        static void RegisterSystems(flecs::world *world);

        void initialize();

        static constexpr std::string_view LocalName()
        {
            return "UIDrawingCanvas";
        }

        Clay_ElementDeclaration buildDecl() override;
        void draw(ClaySdL3RendererData *data) override;

        atmo::core::types::Vector2 screenToCanvas(atmo::core::types::Vector2 screenPos) const;
        atmo::core::types::Vector2 canvasToScreen(atmo::core::types::Vector2i canvasPos) const;

        void validateAndSyncDimensions();

        void saveCanvas();
        void importCanvas(const std::string &path);

        /**
         * @brief Gets the size of a raster image (png, jpg, bmp) without touching the canvas
         *
         * @exception ImportException If the file can't be decoded
         */
        static atmo::core::types::Vector2i ProbeImageSize(const std::string &path);

        /**
         * @brief Slices a raster sprite sheet into a single layer image with one frame per sprite
         *
         * Sprites are read left to right then top to bottom, starting at the top left corner.
         * Everything is checked before the image is replaced, so it is left untouched when this throws.
         *
         * @param frameW Width of a sprite
         * @param frameH Height of a sprite
         * @param frameCount Number of sprites to take, must fit in the sheet and in the .atmo frame limit
         * @exception ImportException If the file can't be decoded or the slicing doesn't fit in the sheet
         */
        void importSpriteSheet(const std::string &path, int frameW, int frameH, int frameCount);

        /**
         * @brief Blends the given layers and writes every frame side by side, left to right, in a raster sprite sheet
         *
         * The sheet is a single row of frames, so importSpriteSheet reads it back with the same frame size.
         * The format (png, bmp, jpg) is taken from the extension of path.
         *
         * @param path Disk path of the sheet, overwritten if it exists
         * @param layerNames Layers to blend, the others are left out whatever their visibility
         * @return false if the sheet could not be created or written
         */
        bool exportSpriteSheet(const std::string &path, const std::vector<std::string> &layerNames);

        void initPixelBuffer(int w, int h);

        void resizeCanvas(int width, int heigth);

    private:
        /**
         * @brief Decodes a png, jpg or bmp file into an RGBA32 surface, the caller destroys it
         *
         * @exception ImportException If the extension isn't supported or the file can't be decoded
         */
        static SDL_Surface *LoadRgbaSurface(const std::string &path);

        /**
         * @brief Picks the format matching the extension of path, png, bmp or jpg
         */
        static components::UIDrawingCanvas::ExportFormat FormatFromPath(const std::string &path);

        void rebuildCheckboard();
        void imageReloaded(int w, int h);
        std::vector<std::vector<atmo::core::types::Color>> *currentFrame();

        void paintPixel(const atmo::core::types::Vector2i &pos, const atmo::core::types::Color &color);
        void
        paintCapsule(const atmo::core::types::Vector2 &from, const atmo::core::types::Vector2 &to, int brush_radius, const atmo::core::types::Color &color);

        float computeFitScale(const components::UIDrawingCanvas &comp) const;
        SDL_FRect computeTextureRect(const components::UIDrawingCanvas &comp) const;
        bool isInsideTextureRect(atmo::core::types::Vector2 screenPos, const components::UIDrawingCanvas &comp) const;

        void handleZoom(const atmo::core::types::Vector2 &mousePosInScreen);
        void handlePan(const atmo::core::types::Vector2 &mousePosInScreen);
        void handleDrawing(const atmo::core::types::Vector2 &mousePosInScreen, const atmo::core::types::Vector2 &mousePosInCanvas);
        void render(SDL_Renderer *renderer);
        void flushPixelsToTexture(SDL_Renderer *renderer);

        void clampOffset(components::UIDrawingCanvas &comp);
    };
} // namespace atmo::core::ecs::entities
