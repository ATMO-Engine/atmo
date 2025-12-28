#include <exception>
#include <stdexcept>
#include "SDL3_ttf/SDL_ttf.h"

#include "core/resource/loaders/font_loader.hpp"

namespace atmo
{
    namespace core
    {
        namespace resource
        {
            FontLoader::FontLoader() {}

            FontLoader::~FontLoader() {}

            TTF_Font *FontLoader::load(const std::string &path)
            {
                try {
                    TTF_Font *res = TTF_OpenFont(path.c_str(), 24); // 24 is the font size in pixels
                    if (!res) {
                        throw std::runtime_error("Failed to load font: " + path);
                    } else {
                        return res;
                    }
                } catch (const std::exception &e) {
                    throw e;
                }
            }

            void FontLoader::destroy(TTF_Font *res)
            {
                if (res != nullptr) {
                    TTF_CloseFont(res); // TODO: Implementer avec le système de caching (retirer la ressource du
                                          // vecteur et l'envoyer dans le cache)
                }
                res = nullptr;
            }
        } // namespace resource
    } // namespace core
} // namespace atmo
// namespace atmo
