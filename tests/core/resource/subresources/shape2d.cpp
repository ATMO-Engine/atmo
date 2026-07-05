#include <memory>

#include <catch2/catch_test_macros.hpp>

#include "core/ecs/entities/2d/physics_2d/body_2d/body_2d.hpp"
#include "core/resource/subresource_registry.hpp"
#include "core/resource/subresources/2d/shape/circle_shape2d.hpp"
#include "core/resource/subresources/2d/shape/rectangle_shape2d.hpp"
#include "glaze/glaze.hpp"

using namespace atmo::core::resource::resources;
using atmo::core::ecs::entities::Body2d;

TEST_CASE("CircleShape2d round-trips through serialize/deserialize", "[subresource][shape2d]")
{
    CircleShape2d shape;
    shape.setRadius(42.5f);

    CircleShape2d reloaded;
    reloaded.deserialize(shape.serialize());

    REQUIRE(reloaded.getRadius() == 42.5f);
}

TEST_CASE("RectangleShape2d round-trips through serialize/deserialize", "[subresource][shape2d]")
{
    RectangleShape2d shape;
    shape.setSize({ 12.0f, 34.0f });

    RectangleShape2d reloaded;
    reloaded.deserialize(shape.serialize());

    REQUIRE(reloaded.getSize().x == 12.0f);
    REQUIRE(reloaded.getSize().y == 34.0f);
}

TEST_CASE("Body2dData shapes survive a JSON round-trip", "[subresource][shape2d][body2d]")
{
    Body2d::Body2dData data;

    auto circle = atmo::core::resource::SubResourceRegistry::Create<Shape2d>("SubResource::Shape2d::CircleShape2d");
    REQUIRE(circle != nullptr);
    std::static_pointer_cast<CircleShape2d>(circle)->setRadius(7.0f);
    data.shapes.push_back(circle);

    auto rectangle = atmo::core::resource::SubResourceRegistry::Create<Shape2d>("SubResource::Shape2d::RectangleShape2d");
    REQUIRE(rectangle != nullptr);
    std::static_pointer_cast<RectangleShape2d>(rectangle)->setSize({ 5.0f, 6.0f });
    data.shapes.push_back(rectangle);

    auto json = glz::write_json(data);
    REQUIRE(json.has_value());

    Body2d::Body2dData reloaded;
    auto err = glz::read_json(reloaded, *json);
    REQUIRE_FALSE(err);

    REQUIRE(reloaded.shapes.size() == 2);

    auto reloaded_circle = std::dynamic_pointer_cast<CircleShape2d>(reloaded.shapes[0]);
    REQUIRE(reloaded_circle != nullptr);
    REQUIRE(reloaded_circle->getRadius() == 7.0f);

    auto reloaded_rectangle = std::dynamic_pointer_cast<RectangleShape2d>(reloaded.shapes[1]);
    REQUIRE(reloaded_rectangle != nullptr);
    REQUIRE(reloaded_rectangle->getSize().x == 5.0f);
    REQUIRE(reloaded_rectangle->getSize().y == 6.0f);
}
