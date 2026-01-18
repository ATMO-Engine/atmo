#include <catch2/catch_test_macros.hpp>
#include <memory>

#include "core/resource/resource_manager.hpp"

TEST_CASE("Simple Load", "[event]")
{
    atmo::core::resource::ResourceManager::GetInstance().increaseTick(1);

    atmo::core::resource::Handle<SDL_Surface> h = {.assetId = "tests/core/resource/example_assets/atmo.png"};

    atmo::core::resource::ResourceRef<SDL_Surface> res =
        atmo::core::resource::ResourceManager::GetInstance()
            .getResource<SDL_Surface>(h.assetId);

    std::shared_ptr<SDL_Surface> s = res.get();
    REQUIRE(s != nullptr);
}

TEST_CASE("Multiple Load", "[event]")
{
    atmo::core::resource::ResourceManager::GetInstance().increaseTick(1);

    atmo::core::resource::Handle<SDL_Surface> h = {.assetId = "tests/core/resource/example_assets/atmo.png"};
    atmo::core::resource::Handle<SDL_Surface> h1 = {.assetId = "tests/core/resource/example_assets/atmo1.png"};

    atmo::core::resource::ResourceRef<SDL_Surface> res =
        atmo::core::resource::ResourceManager::GetInstance()
            .getResource<SDL_Surface>(h.assetId);
    atmo::core::resource::ResourceRef<SDL_Surface> res1 =
        atmo::core::resource::ResourceManager::GetInstance()
            .getResource<SDL_Surface>(h1.assetId);

    std::shared_ptr<SDL_Surface> s = res.get();
    std::shared_ptr<SDL_Surface> s1 = res1.get();
    REQUIRE((s != nullptr && s1 != nullptr));
}
