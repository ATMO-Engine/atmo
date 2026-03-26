#include <csignal>
#include <cstdlib>

#include "atmo.hpp"

int main(int argc, const char *const *argv)
{
    atmo::core::Engine engine;

    if (int ret = engine.init(argc, argv); ret != 0) {
        return ret == -1 ? 0 : 1;
    }

    engine.start();

    return 0;
}
