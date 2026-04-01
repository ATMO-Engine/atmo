#include <exception>
#include <memory>
#include "SDL3/SDL_error.h"
#include "SDL3_ttf/SDL_ttf.h"

#include "core/resource/loaders/font_loader.hpp"
#include "project/file_system.hpp"

namespace atmo
{
    namespace core
    {
        namespace resource
        {
            FontLoader::FontLoader() {}

            FontLoader::~FontLoader() {}

            std::shared_ptr<TTF_Font> FontLoader::load(const std::string &path)
            {
                TTF_Font *res = nullptr;
                try {
                    auto file = project::FileSystem::OpenFile(path);
                    res = TTF_OpenFontIO(file.toIOStream(), true, 24);
                } catch (const std::exception &e) {
                    throw e;
                }

                if (!res) {
                    throw LoadException(std::format("Failed to load font '{}': {}", path, SDL_GetError()));
                } else {
                    return std::shared_ptr<TTF_Font>(res, [](TTF_Font *f) {
                        if (f) {
                            TTF_CloseFont(f);
                        }
                    });
                }
            }
        } // namespace resource
    } // namespace core
} // namespace atmo
// namespace atmo
