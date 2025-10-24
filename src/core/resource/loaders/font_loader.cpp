#include <exception>
#include <stdexcept>

#include "core/resource/loaders/font_loader.hpp"

namespace atmo
{
    namespace core
    {
        namespace resource
        {
            LoaderRegister<FontLoader> FontLoader::_register("ttf");

            FontLoader::FontLoader() {}

            FontLoader::~FontLoader()
            {
                if (_font != nullptr) {
                    TTF_CloseFont(_font);
                }
            }

            void FontLoader::load(const std::string &path)
            {
                try {
                    _font = TTF_OpenFont(path.c_str(), 24); // 24 is the font size in pixels
                    if (!_font) {
                        throw std::runtime_error("Failed to load font: " + path);
                    }
                } catch (const std::exception &e) {
                    throw e;
                }
            }

            std::any FontLoader::get()
            {
                return std::make_any<TTF_Font *>(_font);
            }

            void FontLoader::destroy()
            {
                if (_font != nullptr) {
                    TTF_CloseFont(_font); // TODO: Implementer avec le système de caching (retirer la ressource du
                                          // vecteur et l'envoyer dans le cache)
                }
                _font = nullptr;
            }
        } // namespace resource
    } // namespace core
} // namespace atmo
// namespace atmo
