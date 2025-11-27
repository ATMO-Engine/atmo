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
                if (m_font != nullptr) {
                    TTF_CloseFont(m_font);
                }
            }

            void FontLoader::load(const std::string &path)
            {
                try {
                    m_font = TTF_OpenFont(path.c_str(), 24); // 24 is the font size in pixels
                    if (!m_font) {
                        throw std::runtime_error("Failed to load font: " + path);
                    }
                } catch (const std::exception &e) {
                    throw e;
                }
            }

            std::any FontLoader::get()
            {
                return std::make_any<TTF_Font *>(m_font);
            }

            void FontLoader::destroy()
            {
                if (m_font != nullptr) {
                    TTF_CloseFont(m_font); // TODO: Implementer avec le système de caching (retirer la ressource du
                                           // vecteur et l'envoyer dans le cache)
                }
                m_font = nullptr;
            }
        } // namespace resource
    } // namespace core
} // namespace atmo
// namespace atmo
