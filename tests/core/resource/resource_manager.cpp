#include <memory>
#include <string>

#include <catch2/catch_test_macros.hpp>

#include "SDL3/SDL_surface.h"

#include "core/resource/auto_register_loader.hpp"
#include "core/resource/resource.hpp"
#include "core/resource/resource_manager.hpp"

using atmo::core::resource::ResourceManager;

namespace
{
    constexpr const char *kAtmoPng = "tests/core/resource/example_asset/atmo.png";
    constexpr const char *kAtmo1Png = "tests/core/resource/example_asset/atmo1.png";

    struct TestResourceType {
        int value = 0;
    };

    class TestResourceLoader : public atmo::core::resource::Resource<TestResourceType>
    {
    public:
        std::shared_ptr<TestResourceType> load(const std::string &path) override
        {
            return std::make_shared<TestResourceType>(TestResourceType{ .value = static_cast<int>(path.size()) });
        }
    };
} // namespace

ATMO_REGISTER_RESOURCE_LOADER(TestResourceType, TestResourceLoader, .display_name = "TestResourceType");

TEST_CASE("getResource dedups by path", "[resource]")
{
    auto a = ResourceManager::GetInstance().getResource<SDL_Surface>(kAtmoPng);
    auto b = ResourceManager::GetInstance().getResource<SDL_Surface>(kAtmoPng);
    REQUIRE(a != nullptr);
    REQUIRE(a.get() == b.get());

    auto c = ResourceManager::GetInstance().getResource<SDL_Surface>(kAtmo1Png);
    REQUIRE(c != nullptr);
    REQUIRE(a.get() != c.get());
}

TEST_CASE("getResource reloads once every owner releases the resource", "[resource]")
{
    std::weak_ptr<SDL_Surface> weak;
    {
        auto res = ResourceManager::GetInstance().getResource<SDL_Surface>(kAtmoPng);
        REQUIRE(res != nullptr);
        weak = res;
    }

    REQUIRE(weak.expired());

    auto res = ResourceManager::GetInstance().getResource<SDL_Surface>(kAtmoPng);
    REQUIRE(res != nullptr);
}

TEST_CASE("A resource type can self-register its loader from its own file", "[resource]")
{
    auto res = ResourceManager::GetInstance().getResource<TestResourceType>("hello");
    REQUIRE(res != nullptr);
    REQUIRE(res->value == 5);
}
