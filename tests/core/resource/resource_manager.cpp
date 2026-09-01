#include <memory>
#include <string>

#include <catch2/catch_test_macros.hpp>

#include "SDL3/SDL_hints.h"
#include "SDL3/SDL_init.h"
#include "SDL3/SDL_render.h"
#include "SDL3/SDL_video.h"

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

TEST_CASE("getResource keys GPU resources by (path, renderer)", "[resource][gpu]")
{
    REQUIRE(SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "dummy"));
    REQUIRE(SDL_Init(SDL_INIT_VIDEO));

    SDL_Window *window1 = SDL_CreateWindow("atmo-test-1", 64, 64, SDL_WINDOW_HIDDEN);
    SDL_Window *window2 = SDL_CreateWindow("atmo-test-2", 64, 64, SDL_WINDOW_HIDDEN);
    REQUIRE(window1 != nullptr);
    REQUIRE(window2 != nullptr);

    SDL_Renderer *renderer1 = SDL_CreateRenderer(window1, nullptr);
    SDL_Renderer *renderer2 = SDL_CreateRenderer(window2, nullptr);
    REQUIRE(renderer1 != nullptr);
    REQUIRE(renderer2 != nullptr);

    {
        auto tex1 = ResourceManager::GetInstance().getResource<SDL_Texture>(kAtmoPng, renderer1);
        auto tex1_again = ResourceManager::GetInstance().getResource<SDL_Texture>(kAtmoPng, renderer1);
        auto tex2 = ResourceManager::GetInstance().getResource<SDL_Texture>(kAtmoPng, renderer2);

        REQUIRE(tex1 != nullptr);
        REQUIRE(tex2 != nullptr);
        REQUIRE(tex1.get() == tex1_again.get());
        REQUIRE(tex1.get() != tex2.get());
    }

    ResourceManager::GetInstance().releaseRenderer(renderer1);
    SDL_DestroyRenderer(renderer1);

    ResourceManager::GetInstance().releaseRenderer(renderer2);
    SDL_DestroyRenderer(renderer2);

    SDL_DestroyWindow(window1);
    SDL_DestroyWindow(window2);
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
}
