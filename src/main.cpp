#include <any>
#include <format>
#include <iostream>
#include <memory>
#include <string>
#include "core/resource/handle.hpp"
#include "core/resource/resource_manager.hpp"

int main(int argc, char **argv)
{
    // spdlog::info("Hello world!");
    // atmo::core::Engine::get_singleton()->run();

    atmo::core::resource::ResourceManager &test = atmo::core::resource::ResourceManager::getInstance();
    std::string path = "test.png";
    atmo::core::resource::Handle h = test.generate(path);
    atmo::core::resource::Handle h1= test.generate(path);
    std::cout << std::format("Type in handle: {} other: {}", (int)h.type, h.index) << std::endl;
    std::cout << std::format("Type in handle: {} other: {}", (int)h1.type, h1.index) << std::endl;
    try {
        std::any result = test.getResource(h);
        std::cout << "Type in std::any: " << result.type().name() << std::endl;
        std::any result1 = test.getResource(h1);
        std::cout << "Type in std::any: " << result1.type().name() << std::endl;

        std::string val = std::any_cast<std::string>(result);
        std::cout << val << std::endl;
        std::string val1 = std::any_cast<std::string>(result1);
        std::cout << val << std::endl;
    }
    catch (const std::exception &e) {
        std::cerr << "Bad cast: " << e.what() << "\n";
    }


    //try {
    //    atmo::core::resource::Bytecode result = std::any_cast<atmo::core::resource::Bytecode>(save);
    //    char *val = *result.data.get();
    //std::cout << "test" << std::endl;
//
    //    for (size_t i = 0; i < result.size; ++i) {
    //        std::cout << std::hex << std::uppercase << val[i];
    //    }
    //    std::cout << "test" << std::endl;
    //}
    //catch (const std::exception &e) {
    //    std::cerr << "Bad cast: " << e.what() << "\n";
    //}
    //
    std::cout << "test" << std::endl;
    return 0;
}
