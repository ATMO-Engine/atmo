#include "core/ecs/entities/window/window.hpp"
#include "scene.hpp"

#include <array>
#include <cmath>
#include <vector>

#include "SDL3/SDL_render.h"
#include "common/utils.hpp"
#include "core/types.hpp"

// Helpers to apply the camera transform to world-space Box2D coordinates.
static inline float editorX(const EditorDebugContext *ctx, float wx) { return wx * ctx->zoom + ctx->pan.x; }
static inline float editorY(const EditorDebugContext *ctx, float wy) { return wy * ctx->zoom + ctx->pan.y; }

static void DrawTransform(b2Transform transform, void *context)
{
    auto *window = static_cast<atmo::core::components::Window *>(context);

    if (window == nullptr)
        return;

    const float k_axis_scale = 0.4f;

    atmo::core::types::Vector2 p1 = transform.p;
    atmo::core::types::Vector2 p2 = transform.p + k_axis_scale * b2Rot_GetXAxis(transform.q);
    atmo::core::types::Vector2 p3 = transform.p + k_axis_scale * b2Rot_GetYAxis(transform.q);

    SDL_SetRenderDrawColor(window->renderer_data.renderer, 255, 0, 0, 255);
    SDL_RenderLine(window->renderer_data.renderer, p1.x, p1.y, p2.x, p2.y);

    SDL_SetRenderDrawColor(window->renderer_data.renderer, 0, 255, 0, 255);
    SDL_RenderLine(window->renderer_data.renderer, p1.x, p1.y, p3.x, p3.y);
}

static void DrawPoint(b2Vec2 p, float size, b2HexColor color, void *context)
{
    auto *window = static_cast<atmo::core::components::Window *>(context);

    if (window == nullptr)
        return;

    atmo::core::types::Color rgba = atmo::core::types::Color::FromHex(color);
    rgba.a = 1.0f;
    std::array<std::uint8_t, 4> rgba_sdl = rgba.toInt<std::array<std::uint8_t, 4>>();

    atmo::core::types::Vector2 vec = p;

    SDL_FRect rect{ vec.x - size / 2, vec.y - size / 2, size, size };

    SDL_SetRenderDrawColor(window->renderer_data.renderer, rgba_sdl[0], rgba_sdl[1], rgba_sdl[2], rgba_sdl[3]);
    SDL_RenderFillRect(window->renderer_data.renderer, &rect);
}

static void DrawSegment(b2Vec2 p1, b2Vec2 p2, b2HexColor color, void *context)
{
    auto *window = static_cast<atmo::core::components::Window *>(context);

    if (window == nullptr)
        return;

    atmo::core::types::Color rgba = atmo::core::types::Color::FromHex(color);
    rgba.a = 1.0f;
    std::array<std::uint8_t, 4> rgba_sdl = rgba.toInt<std::array<std::uint8_t, 4>>();

    atmo::core::types::Vector2 vec1 = p1;
    atmo::core::types::Vector2 vec2 = p2;

    SDL_SetRenderDrawColor(window->renderer_data.renderer, rgba_sdl[0], rgba_sdl[1], rgba_sdl[2], rgba_sdl[3]);
    SDL_RenderLine(window->renderer_data.renderer, vec1.x, vec1.y, vec2.x, vec2.y);
}

static void DrawPolygon(const b2Vec2 *vertices, int vertexCount, b2HexColor color, void *context)
{
    auto *window = static_cast<atmo::core::components::Window *>(context);

    if (window == nullptr || vertices == nullptr || vertexCount < 2)
        return;

    std::vector<SDL_FPoint> points(vertexCount);
    for (int i = 0; i < vertexCount; ++i) {
        atmo::core::types::Vector2 vec = vertices[i];
        points[i] = { vec.x, vec.y };
    }

    atmo::core::types::Color rgba = atmo::core::types::Color::FromHex(color);
    rgba.a = 1.0f;
    std::array<std::uint8_t, 4> rgba_sdl = rgba.toInt<std::array<std::uint8_t, 4>>();

    SDL_SetRenderDrawColor(window->renderer_data.renderer, rgba_sdl[0], rgba_sdl[1], rgba_sdl[2], rgba_sdl[3]);
    SDL_RenderLines(window->renderer_data.renderer, points.data(), vertexCount);
};

static void DrawSolidPolygon(b2Transform transform, const b2Vec2 *vertices, int vertexCount, float radius, b2HexColor color, void *context)
{
    auto *window = static_cast<atmo::core::components::Window *>(context);

    if (window == nullptr || vertices == nullptr || vertexCount < 3)
        return;

    SDL_SetRenderDrawBlendMode(window->renderer_data.renderer, SDL_BLENDMODE_BLEND);

    std::vector<SDL_Vertex> points(vertexCount);

    atmo::core::types::Color rgba = atmo::core::types::Color::FromHex(color);
    rgba.a = 0.35f;

    for (int i = 0; i < vertexCount; ++i) {
        b2Vec2 world_point = b2TransformPoint(transform, vertices[i]);
        atmo::core::types::Vector2 vec = world_point;
        points[i] = SDL_Vertex{ .position = { vec.x, vec.y }, .color = rgba.toFloat<SDL_FColor>(), .tex_coord = { 0, 0 } };
    }

    std::vector<int> indices;
    indices.reserve(static_cast<size_t>(vertexCount - 2) * 3);
    for (int i = 1; i < vertexCount - 1; ++i) {
        indices.push_back(0);
        indices.push_back(i);
        indices.push_back(i + 1);
    }

    SDL_RenderGeometry(window->renderer_data.renderer, nullptr, points.data(), vertexCount, indices.data(), static_cast<int>(indices.size()));

    atmo::core::types::Color outline_rgba = atmo::core::types::Color::FromHex(color);
    outline_rgba.a = 1.0f;
    std::array<std::uint8_t, 4> outline_rgba_sdl = outline_rgba.toInt<std::array<std::uint8_t, 4>>();
    SDL_SetRenderDrawColor(window->renderer_data.renderer, outline_rgba_sdl[0], outline_rgba_sdl[1], outline_rgba_sdl[2], outline_rgba_sdl[3]);

    std::vector<SDL_FPoint> outline_points(static_cast<size_t>(vertexCount) + 1);
    for (int i = 0; i < vertexCount; ++i) {
        outline_points[static_cast<size_t>(i)] = points[static_cast<size_t>(i)].position;
    }
    outline_points[static_cast<size_t>(vertexCount)] = points[0].position;
    SDL_RenderLines(window->renderer_data.renderer, outline_points.data(), vertexCount + 1);

    const float radius_px = atmo::common::math::MeterToPixel(radius);
    if (radius_px > 0.0f) {
        constexpr int k_segments = 12;
        std::vector<SDL_Vertex> circle_vertices(static_cast<size_t>(k_segments) + 2);
        std::vector<int> circle_indices(static_cast<size_t>(k_segments) * 3);

        for (int v = 0; v < vertexCount; ++v) {
            const SDL_FPoint c = points[static_cast<size_t>(v)].position;

            circle_vertices[0] = SDL_Vertex{ .position = c, .color = rgba.toFloat<SDL_FColor>(), .tex_coord = { 0, 0 } };

            for (int s = 0; s <= k_segments; ++s) {
                const float a = 2.0f * atmo::common::math::PI * (static_cast<float>(s) / static_cast<float>(k_segments));
                const float x = c.x + radius_px * std::cos(a);
                const float y = c.y + radius_px * std::sin(a);
                circle_vertices[static_cast<size_t>(s) + 1] = SDL_Vertex{ .position = { x, y }, .color = rgba.toFloat<SDL_FColor>(), .tex_coord = { 0, 0 } };
            }

            for (int s = 0; s < k_segments; ++s) {
                const size_t base = static_cast<size_t>(s) * 3;
                circle_indices[base + 0] = 0;
                circle_indices[base + 1] = s + 1;
                circle_indices[base + 2] = s + 2;
            }

            SDL_RenderGeometry(
                window->renderer_data.renderer,
                nullptr,
                circle_vertices.data(),
                static_cast<int>(circle_vertices.size()),
                circle_indices.data(),
                static_cast<int>(circle_indices.size()));
        }
    }
}

static void DrawSolidCircle(b2Transform transform, float radius, b2HexColor color, void *context)
{
    auto *window = static_cast<atmo::core::components::Window *>(context);

    if (window == nullptr || radius <= 0.0f)
        return;

    SDL_SetRenderDrawBlendMode(window->renderer_data.renderer, SDL_BLENDMODE_BLEND);

    atmo::core::types::Color rgba = atmo::core::types::Color::FromHex(color);
    rgba.a = 0.35f;

    const atmo::core::types::Vector2 center_px = transform.p;
    const float radius_px = atmo::common::math::MeterToPixel(radius);

    constexpr int k_segments = 24;

    std::vector<SDL_Vertex> vertices(static_cast<size_t>(k_segments) + 2);
    std::vector<int> indices(static_cast<size_t>(k_segments) * 3);

    vertices[0] = SDL_Vertex{ .position = { center_px.x, center_px.y }, .color = rgba.toFloat<SDL_FColor>(), .tex_coord = { 0, 0 } };

    for (int i = 0; i <= k_segments; ++i) {
        const float a = 2.0f * atmo::common::math::PI * (static_cast<float>(i) / static_cast<float>(k_segments));
        const float x = center_px.x + radius_px * std::cos(a);
        const float y = center_px.y + radius_px * std::sin(a);
        vertices[static_cast<size_t>(i) + 1] = SDL_Vertex{ .position = { x, y }, .color = rgba.toFloat<SDL_FColor>(), .tex_coord = { 0, 0 } };
    }

    for (int i = 0; i < k_segments; ++i) {
        const size_t base = static_cast<size_t>(i) * 3;
        indices[base + 0] = 0;
        indices[base + 1] = i + 1;
        indices[base + 2] = i + 2;
    }

    SDL_RenderGeometry(
        window->renderer_data.renderer, nullptr, vertices.data(), static_cast<int>(vertices.size()), indices.data(), static_cast<int>(indices.size()));

    atmo::core::types::Color outline_rgba = atmo::core::types::Color::FromHex(color);
    outline_rgba.a = 1.0f;
    std::array<std::uint8_t, 4> outline_rgba_sdl = outline_rgba.toInt<std::array<std::uint8_t, 4>>();
    SDL_SetRenderDrawColor(window->renderer_data.renderer, outline_rgba_sdl[0], outline_rgba_sdl[1], outline_rgba_sdl[2], outline_rgba_sdl[3]);

    std::vector<SDL_FPoint> outline_points(static_cast<size_t>(k_segments) + 1);
    for (int i = 0; i <= k_segments; ++i) {
        outline_points[static_cast<size_t>(i)] = vertices[static_cast<size_t>(i) + 1].position;
    }
    SDL_RenderLines(window->renderer_data.renderer, outline_points.data(), k_segments + 1);
}


// ---------------------------------------------------------------------------
// Editor-specific draw callbacks — identical logic but with camera transform.
// ---------------------------------------------------------------------------

static void DrawTransform_Editor(b2Transform transform, void *context)
{
    auto *ctx = static_cast<EditorDebugContext *>(context);
    if (!ctx || !ctx->renderer)
        return;

    const float k = 0.4f;
    atmo::core::types::Vector2 p1 = transform.p;
    atmo::core::types::Vector2 p2 = transform.p + k * b2Rot_GetXAxis(transform.q);
    atmo::core::types::Vector2 p3 = transform.p + k * b2Rot_GetYAxis(transform.q);

    SDL_SetRenderDrawColor(ctx->renderer, 255, 0, 0, 255);
    SDL_RenderLine(ctx->renderer, editorX(ctx, p1.x), editorY(ctx, p1.y), editorX(ctx, p2.x), editorY(ctx, p2.y));

    SDL_SetRenderDrawColor(ctx->renderer, 0, 255, 0, 255);
    SDL_RenderLine(ctx->renderer, editorX(ctx, p1.x), editorY(ctx, p1.y), editorX(ctx, p3.x), editorY(ctx, p3.y));
}

static void DrawPoint_Editor(b2Vec2 p, float size, b2HexColor color, void *context)
{
    auto *ctx = static_cast<EditorDebugContext *>(context);
    if (!ctx || !ctx->renderer)
        return;

    atmo::core::types::Color rgba = atmo::core::types::Color::FromHex(color);
    rgba.a = 1.0f;
    std::array<std::uint8_t, 4> c = rgba.toInt<std::array<std::uint8_t, 4>>();

    float sx = editorX(ctx, p.x);
    float sy = editorY(ctx, p.y);
    float s = size * ctx->zoom;
    SDL_FRect rect{ sx - s * 0.5f, sy - s * 0.5f, s, s };

    SDL_SetRenderDrawColor(ctx->renderer, c[0], c[1], c[2], c[3]);
    SDL_RenderFillRect(ctx->renderer, &rect);
}

static void DrawSegment_Editor(b2Vec2 p1, b2Vec2 p2, b2HexColor color, void *context)
{
    auto *ctx = static_cast<EditorDebugContext *>(context);
    if (!ctx || !ctx->renderer)
        return;

    atmo::core::types::Color rgba = atmo::core::types::Color::FromHex(color);
    rgba.a = 1.0f;
    std::array<std::uint8_t, 4> c = rgba.toInt<std::array<std::uint8_t, 4>>();

    SDL_SetRenderDrawColor(ctx->renderer, c[0], c[1], c[2], c[3]);
    SDL_RenderLine(ctx->renderer, editorX(ctx, p1.x), editorY(ctx, p1.y), editorX(ctx, p2.x), editorY(ctx, p2.y));
}

static void DrawPolygon_Editor(const b2Vec2 *vertices, int vertexCount, b2HexColor color, void *context)
{
    auto *ctx = static_cast<EditorDebugContext *>(context);
    if (!ctx || !ctx->renderer || !vertices || vertexCount < 2)
        return;

    std::vector<SDL_FPoint> points(static_cast<size_t>(vertexCount) + 1);
    for (int i = 0; i < vertexCount; ++i)
        points[static_cast<size_t>(i)] = { editorX(ctx, vertices[i].x), editorY(ctx, vertices[i].y) };
    points[static_cast<size_t>(vertexCount)] = points[0];

    atmo::core::types::Color rgba = atmo::core::types::Color::FromHex(color);
    rgba.a = 1.0f;
    std::array<std::uint8_t, 4> c = rgba.toInt<std::array<std::uint8_t, 4>>();

    SDL_SetRenderDrawColor(ctx->renderer, c[0], c[1], c[2], c[3]);
    SDL_RenderLines(ctx->renderer, points.data(), vertexCount + 1);
}

static void DrawSolidPolygon_Editor(b2Transform transform, const b2Vec2 *vertices, int vertexCount, float radius, b2HexColor color, void *context)
{
    auto *ctx = static_cast<EditorDebugContext *>(context);
    if (!ctx || !ctx->renderer || !vertices || vertexCount < 3)
        return;

    SDL_SetRenderDrawBlendMode(ctx->renderer, SDL_BLENDMODE_BLEND);

    atmo::core::types::Color rgba = atmo::core::types::Color::FromHex(color);
    rgba.a = 0.35f;

    std::vector<SDL_Vertex> points(static_cast<size_t>(vertexCount));
    for (int i = 0; i < vertexCount; ++i) {
        b2Vec2 wp = b2TransformPoint(transform, vertices[i]);
        points[static_cast<size_t>(i)] = SDL_Vertex{
            .position = { editorX(ctx, wp.x), editorY(ctx, wp.y) },
            .color = rgba.toFloat<SDL_FColor>(),
            .tex_coord = { 0, 0 }
        };
    }

    std::vector<int> indices;
    indices.reserve(static_cast<size_t>(vertexCount - 2) * 3);
    for (int i = 1; i < vertexCount - 1; ++i) {
        indices.push_back(0);
        indices.push_back(i);
        indices.push_back(i + 1);
    }

    SDL_RenderGeometry(ctx->renderer, nullptr, points.data(), vertexCount, indices.data(), static_cast<int>(indices.size()));

    atmo::core::types::Color outline = atmo::core::types::Color::FromHex(color);
    outline.a = 1.0f;
    std::array<std::uint8_t, 4> oc = outline.toInt<std::array<std::uint8_t, 4>>();
    SDL_SetRenderDrawColor(ctx->renderer, oc[0], oc[1], oc[2], oc[3]);

    std::vector<SDL_FPoint> outline_pts(static_cast<size_t>(vertexCount) + 1);
    for (int i = 0; i < vertexCount; ++i)
        outline_pts[static_cast<size_t>(i)] = points[static_cast<size_t>(i)].position;
    outline_pts[static_cast<size_t>(vertexCount)] = points[0].position;
    SDL_RenderLines(ctx->renderer, outline_pts.data(), vertexCount + 1);

    const float radius_px = atmo::common::math::MeterToPixel(radius) * ctx->zoom;
    if (radius_px > 0.0f) {
        constexpr int k_segments = 12;
        std::vector<SDL_Vertex> cv(static_cast<size_t>(k_segments) + 2);
        std::vector<int> ci(static_cast<size_t>(k_segments) * 3);

        for (int v = 0; v < vertexCount; ++v) {
            const SDL_FPoint c = points[static_cast<size_t>(v)].position;
            cv[0] = SDL_Vertex{ .position = c, .color = rgba.toFloat<SDL_FColor>(), .tex_coord = { 0, 0 } };
            for (int s = 0; s <= k_segments; ++s) {
                const float a = 2.0f * atmo::common::math::PI * (static_cast<float>(s) / static_cast<float>(k_segments));
                cv[static_cast<size_t>(s) + 1] = SDL_Vertex{
                    .position = { c.x + radius_px * std::cos(a), c.y + radius_px * std::sin(a) },
                    .color = rgba.toFloat<SDL_FColor>(),
                    .tex_coord = { 0, 0 }
                };
            }
            for (int s = 0; s < k_segments; ++s) {
                const size_t base = static_cast<size_t>(s) * 3;
                ci[base + 0] = 0;
                ci[base + 1] = s + 1;
                ci[base + 2] = s + 2;
            }
            SDL_RenderGeometry(ctx->renderer, nullptr, cv.data(), static_cast<int>(cv.size()), ci.data(), static_cast<int>(ci.size()));
        }
    }
}

static void DrawSolidCircle_Editor(b2Transform transform, float radius, b2HexColor color, void *context)
{
    auto *ctx = static_cast<EditorDebugContext *>(context);
    if (!ctx || !ctx->renderer || radius <= 0.0f)
        return;

    SDL_SetRenderDrawBlendMode(ctx->renderer, SDL_BLENDMODE_BLEND);

    atmo::core::types::Color rgba = atmo::core::types::Color::FromHex(color);
    rgba.a = 0.35f;

    const float cx = editorX(ctx, static_cast<float>(transform.p.x));
    const float cy = editorY(ctx, static_cast<float>(transform.p.y));
    const float r = atmo::common::math::MeterToPixel(radius) * ctx->zoom;

    constexpr int k_segments = 24;
    std::vector<SDL_Vertex> verts(static_cast<size_t>(k_segments) + 2);
    std::vector<int> indices(static_cast<size_t>(k_segments) * 3);

    verts[0] = SDL_Vertex{ .position = { cx, cy }, .color = rgba.toFloat<SDL_FColor>(), .tex_coord = { 0, 0 } };
    for (int i = 0; i <= k_segments; ++i) {
        const float a = 2.0f * atmo::common::math::PI * (static_cast<float>(i) / static_cast<float>(k_segments));
        verts[static_cast<size_t>(i) + 1] = SDL_Vertex{
            .position = { cx + r * std::cos(a), cy + r * std::sin(a) },
            .color = rgba.toFloat<SDL_FColor>(),
            .tex_coord = { 0, 0 }
        };
    }
    for (int i = 0; i < k_segments; ++i) {
        const size_t base = static_cast<size_t>(i) * 3;
        indices[base + 0] = 0;
        indices[base + 1] = i + 1;
        indices[base + 2] = i + 2;
    }

    SDL_RenderGeometry(ctx->renderer, nullptr, verts.data(), static_cast<int>(verts.size()), indices.data(), static_cast<int>(indices.size()));

    atmo::core::types::Color outline = atmo::core::types::Color::FromHex(color);
    outline.a = 1.0f;
    std::array<std::uint8_t, 4> oc = outline.toInt<std::array<std::uint8_t, 4>>();
    SDL_SetRenderDrawColor(ctx->renderer, oc[0], oc[1], oc[2], oc[3]);

    std::vector<SDL_FPoint> outline_pts(static_cast<size_t>(k_segments) + 1);
    for (int i = 0; i <= k_segments; ++i)
        outline_pts[static_cast<size_t>(i)] = verts[static_cast<size_t>(i) + 1].position;
    SDL_RenderLines(ctx->renderer, outline_pts.data(), k_segments + 1);
}

namespace atmo::core::ecs::entities
{
    void Scene::SetupDebugDraw(b2DebugDraw *debugDraw)
    {
        debugDraw->drawShapes = true;
        debugDraw->drawMass = true;

        debugDraw->DrawTransformFcn = DrawTransform;
        debugDraw->DrawPointFcn = DrawPoint;
        debugDraw->DrawSegmentFcn = DrawSegment;
        debugDraw->DrawPolygonFcn = DrawPolygon;
        debugDraw->DrawSolidPolygonFcn = DrawSolidPolygon;
        debugDraw->DrawSolidCircleFcn = DrawSolidCircle;
    }

    void Scene::SetupEditorDebugDraw(b2DebugDraw *debugDraw)
    {
        debugDraw->drawShapes = true;
        debugDraw->drawMass = true;

        debugDraw->DrawTransformFcn = DrawTransform_Editor;
        debugDraw->DrawPointFcn = DrawPoint_Editor;
        debugDraw->DrawSegmentFcn = DrawSegment_Editor;
        debugDraw->DrawPolygonFcn = DrawPolygon_Editor;
        debugDraw->DrawSolidPolygonFcn = DrawSolidPolygon_Editor;
        debugDraw->DrawSolidCircleFcn = DrawSolidCircle_Editor;
    }
} // namespace atmo::core::ecs::entities
