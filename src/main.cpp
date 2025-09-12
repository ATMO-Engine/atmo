#include "atmo.hpp"
#include "core/resource/resource_manager.hpp"

int main(int argc, char **argv)
{
    // spdlog::info("Hello world!");
    // atmo::core::Engine::get_singleton()->run();
    atmo::core::resource::ResourceManager test;

    std::string path = "test.png";

    test.getResources(path);
    return 0;
}
