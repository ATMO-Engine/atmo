#include <exception>
#include <memory>
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

            std::shared_ptr<TTF_Font> FontLoader::load(const std::string &path)
            {
                try {
                    TTF_Font *res = TTF_OpenFont(path.c_str(), 24); // 24 is the font size in pixels
                    if (!res) {
                        throw std::runtime_error("Failed to load font: " + path);
                    } else {
                        return std::shared_ptr<TTF_Font>(
                            res,
                            [](TTF_Font *f) {
                                if (f) {
                                    TTF_CloseFont(f);
                                }
                            }
                        );
                    }
                } catch (const std::exception &e) {
                    throw e;
                }
            }
        } // namespace resource
    } // namespace core
} // namespace atmo
// namespace atmo
