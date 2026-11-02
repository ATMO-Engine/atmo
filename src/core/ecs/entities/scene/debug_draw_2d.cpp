#include "scene.hpp"

#include <array>
#include <cmath>
#include <vector>

#include "common/utils.hpp"
#include "core/types.hpp"
#include "server/rendering/rendering_server.hpp"

namespace
{
    using atmo::core::types::Color;
    using atmo::core::types::Vector2;
    using atmo::server::rendering::CanvasItemRid;
    using atmo::server::rendering::RenderingServer;

    constexpr float fill_alpha = 0.35f;
    constexpr float axis_scale = 0.4f;

    CanvasItemRid Item(void *context)
    {
        return context ? *static_cast<CanvasItemRid *>(context) : CanvasItemRid{};
    }

    Color Opaque(b2HexColor hex)
    {
        Color color = Color::FromHex(hex);
        color.a = 1.0f;
        return color;
    }

    Color Translucent(b2HexColor hex)
    {
        Color color = Color::FromHex(hex);
        color.a = fill_alpha;
        return color;
    }

    std::vector<Vector2> Circle(Vector2 center, float radius, int segments)
    {
        std::vector<Vector2> points;
        points.reserve(static_cast<std::size_t>(segments));
        for (int i = 0; i < segments; ++i) {
            const float angle = 2.0f * atmo::common::math::PI * (static_cast<float>(i) / static_cast<float>(segments));
            points.emplace_back(center.x + radius * std::cos(angle), center.y + radius * std::sin(angle));
        }
        return points;
    }

    void FillAndOutline(CanvasItemRid item, const std::vector<Vector2> &points, b2HexColor hex)
    {
        auto &server = RenderingServer::Instance();
        const Color fill = Translucent(hex);
        server.canvasItemAddPolygon(item, points, { &fill, 1 });
        server.canvasItemAddPolyline(item, points, Opaque(hex), 1.0f, true);
    }

    void DrawTransform(b2Transform transform, void *context)
    {
        const CanvasItemRid item = Item(context);
        if (!item.isValid())
            return;

        const Vector2 origin = transform.p;
        const std::array x_axis{ origin, Vector2(transform.p + axis_scale * b2Rot_GetXAxis(transform.q)) };
        const std::array y_axis{ origin, Vector2(transform.p + axis_scale * b2Rot_GetYAxis(transform.q)) };

        auto &server = RenderingServer::Instance();
        server.canvasItemAddPolyline(item, x_axis, Color(1.0f, 0.0f, 0.0f, 1.0f));
        server.canvasItemAddPolyline(item, y_axis, Color(0.0f, 1.0f, 0.0f, 1.0f));
    }

    void DrawPoint(b2Vec2 p, float size, b2HexColor color, void *context)
    {
        const CanvasItemRid item = Item(context);
        if (!item.isValid())
            return;

        const Vector2 center = p;
        RenderingServer::Instance().canvasItemAddRect(item, { { center.x - size * 0.5f, center.y - size * 0.5f }, { size, size } }, Opaque(color));
    }

    void DrawSegment(b2Vec2 p1, b2Vec2 p2, b2HexColor color, void *context)
    {
        const CanvasItemRid item = Item(context);
        if (!item.isValid())
            return;

        const std::array points{ Vector2(p1), Vector2(p2) };
        RenderingServer::Instance().canvasItemAddPolyline(item, points, Opaque(color));
    }

    void DrawPolygon(const b2Vec2 *vertices, int vertex_count, b2HexColor color, void *context)
    {
        const CanvasItemRid item = Item(context);
        if (!item.isValid() || !vertices || vertex_count < 2)
            return;

        std::vector<Vector2> points(vertices, vertices + vertex_count);
        RenderingServer::Instance().canvasItemAddPolyline(item, points, Opaque(color), 1.0f, true);
    }

    void DrawSolidPolygon(b2Transform transform, const b2Vec2 *vertices, int vertex_count, float radius, b2HexColor color, void *context)
    {
        const CanvasItemRid item = Item(context);
        if (!item.isValid() || !vertices || vertex_count < 3)
            return;

        std::vector<Vector2> points;
        points.reserve(static_cast<std::size_t>(vertex_count));
        for (int i = 0; i < vertex_count; ++i) points.emplace_back(b2TransformPoint(transform, vertices[i]));
        FillAndOutline(item, points, color);

        const float radius_px = atmo::common::math::MeterToPixel(radius);
        if (radius_px <= 0.0f)
            return;

        auto &server = RenderingServer::Instance();
        const Color fill = Translucent(color);
        for (const Vector2 &corner : points) server.canvasItemAddPolygon(item, Circle(corner, radius_px, 12), { &fill, 1 });
    }

    void DrawSolidCircle(b2Transform transform, float radius, b2HexColor color, void *context)
    {
        const CanvasItemRid item = Item(context);
        if (!item.isValid() || radius <= 0.0f)
            return;

        FillAndOutline(item, Circle(transform.p, atmo::common::math::MeterToPixel(radius), 24), color);
    }
} // namespace

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
} // namespace atmo::core::ecs::entities
