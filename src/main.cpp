#include <iostream>
#include "core/resource/loaders/script_loader.hpp"
#include "core/resource/resource_manager.hpp"

int main(int argc, char **argv)
{
    // spdlog::info("Hello world!");
    // atmo::core::Engine::get_singleton()->run();
    atmo::core::resource::ResourceManager test;

    std::string path = "test.lua";
    std::any save = test.getResources(path);
    try {
        atmo::core::resource::Bytecode result = std::any_cast<atmo::core::resource::Bytecode>(save);
        char *val = *result.data.get();

        for (size_t i = 0; i < result.size; ++i) {
            std::cout << std::hex << std::uppercase << val[i];
        }
    }
    catch (const std::exception &e) {
        std::cerr << "Bad cast: " << e.what() << "\n";
    }
    return 0;
}
