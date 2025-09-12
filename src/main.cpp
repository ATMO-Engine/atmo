#include <iostream>
#include "core/resource/resource_manager.hpp"

int main(int argc, char **argv)
{
    // spdlog::info("Hello world!");
    // atmo::core::Engine::get_singleton()->run();
    atmo::core::resource::ResourceManager test;

    std::string path = "test.lua";
    std::any save = test.getResources(path);
    try {
        std::shared_ptr<char *> extracted = std::any_cast<std::shared_ptr<char *>>(save);
        char *val = *extracted;
        for (int i = 0; val[i] != '\0'; i++) {
            std::cout << val[i] << "(" << (int)val[i] << ")" << std::endl;
        }
    }
    catch (const std::exception &e) {
        std::cerr << "Bad cast: " << e.what() << "\n";
    }
    return 0;
}
