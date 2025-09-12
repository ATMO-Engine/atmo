#include <iostream>
#include "core/resource/resource_manager.hpp"

int main(int argc, char **argv)
{
    // spdlog::info("Hello world!");
    // atmo::core::Engine::get_singleton()->run();
    std::cout << "0" << std::endl;
    atmo::core::resource::ResourceManager test;
    std::cout << "1" << std::endl;

    std::string path = "caca.png";
    std::cout << "2" << std::endl;

    test.getResources(path);
    std::cout << "3" << std::endl;
    return 0;
}
