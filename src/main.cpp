#include "atmo.hpp"

int main(int argc, char **argv)
{
    spdlog::info("Hello world!");
    atmo::core::Engine::get_singleton()->run();
    return 0;
}
