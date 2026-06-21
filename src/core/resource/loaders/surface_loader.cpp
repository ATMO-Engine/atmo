#include "surface_loader.hpp"
#include <exception>
#include <memory>
#include "SDL3_image/SDL_image.h"
#include "project/file_system.hpp"

namespace atmo
{
    namespace core
    {
        namespace resource
        {
            SurfaceLoader::SurfaceLoader() {}

            SurfaceLoader::~SurfaceLoader() {}

            std::shared_ptr<SDL_Surface> SurfaceLoader::load(const std::string &path)
            {
                auto file = project::FileSystem::OpenFile(path);
                SDL_Surface *surface = IMG_Load_IO(file.toIOStream(), true);

                if (!surface) {
                    throw LoadException(std::string("Failed to load image: ") + SDL_GetError());
                }
                return std::shared_ptr<SDL_Surface>(surface, [](SDL_Surface *s) {
                    if (s) {
                        SDL_DestroySurface(s);
                    }
                });
            }

            const std::string SurfaceLoader::resourceTypeName()
            {
                return "Image (CPU & RAM)";
            }
        } // namespace resource
    } // namespace core
} // namespace atmo
