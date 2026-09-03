#include <filesystem>
#include <fstream>
#include <memory>
#include <string>

#include <catch2/catch_test_macros.hpp>

#include "core/resource/resource_file.hpp"
#include "core/resource/subresources/2d/shape/circle_shape2d.hpp"
#include "glaze/glaze.hpp"

using namespace atmo::core::resource;
using namespace atmo::core::resource::resources;

struct TestMain {
    std::string name;
};

TEST_CASE("ResourceFile round-trips main + subresources through JSON", "[resource][subresource]")
{
    ResourceFile<TestMain> file;
    file.main.name = "test level";

    auto circle = SubResourceRegistry::Create<Shape2d>("SubResource::Shape2d::CircleShape2d");
    REQUIRE(circle != nullptr);
    std::static_pointer_cast<CircleShape2d>(circle)->setRadius(9.0f);
    file.subresources.push_back({ .id = "hitbox_a", .type = std::string(circle->getTypeName()), .data = circle->serialize() });

    auto json = glz::write_json(file);
    REQUIRE(json.has_value());

    ResourceFile<TestMain> reloaded;
    auto err = glz::read_json(reloaded, *json);
    REQUIRE_FALSE(err);

    REQUIRE(reloaded.main.name == "test level");
    REQUIRE(reloaded.subresources.size() == 1);
    REQUIRE(reloaded.subresources[0].id == "hitbox_a");

    auto rebuilt = SubResourceRegistry::Create<Shape2d>(reloaded.subresources[0].type);
    REQUIRE(rebuilt != nullptr);
    rebuilt->deserialize(reloaded.subresources[0].data);

    auto rebuilt_circle = std::dynamic_pointer_cast<CircleShape2d>(rebuilt);
    REQUIRE(rebuilt_circle != nullptr);
    REQUIRE(rebuilt_circle->getRadius() == 9.0f);
}

TEST_CASE("ResourceFile round-trips through the BEVE binary format", "[resource][subresource]")
{
    ResourceFile<TestMain> file;
    file.main.name = "binary level";
    file.subresources.push_back({ .id = "a", .type = "x", .data = "{}" });

    std::string buffer;
    auto write_err = glz::write_beve(file, buffer);
    REQUIRE_FALSE(write_err);

    ResourceFile<TestMain> reloaded;
    auto read_err = glz::read_beve(reloaded, buffer);
    REQUIRE_FALSE(read_err);

    REQUIRE(reloaded.main.name == "binary level");
    REQUIRE(reloaded.subresources.size() == 1);
    REQUIRE(reloaded.subresources[0].id == "a");
}

TEST_CASE("LoadResourceFile and ResolveSubResource read a real file from disk", "[resource][subresource]")
{
    const std::string path = (std::filesystem::temp_directory_path() / "atmo_resource_file_test.json").string();

    ResourceFile<TestMain> file;
    file.main.name = "on disk";

    auto circle = SubResourceRegistry::Create<Shape2d>("SubResource::Shape2d::CircleShape2d");
    REQUIRE(circle != nullptr);
    std::static_pointer_cast<CircleShape2d>(circle)->setRadius(3.0f);
    file.subresources.push_back({ .id = "hitbox_a", .type = std::string(circle->getTypeName()), .data = circle->serialize() });

    auto json = glz::write_json(file);
    REQUIRE(json.has_value());

    {
        std::ofstream out(path);
        out << *json;
    }

    auto loaded = LoadResourceFile<TestMain>(path);
    REQUIRE(loaded.main.name == "on disk");
    REQUIRE(loaded.subresources.size() == 1);

    auto resolved = ResolveSubResource<Shape2d, TestMain>(path + "#hitbox_a");
    auto resolved_circle = std::dynamic_pointer_cast<CircleShape2d>(resolved);
    REQUIRE(resolved_circle != nullptr);
    REQUIRE(resolved_circle->getRadius() == 3.0f);

    std::filesystem::remove(path);
}
