#include <memory>
#include <spdlog/spdlog.h>
#include "core/resource/loaders/font_loader.hpp"
#include "core/resource/loaders/script_loader.hpp"
#include "core/resource/loaders/surface_loader.hpp"
#include "core/resource/loaders/texture_loader.hpp"
#include "core/resource/resource.hpp"

namespace atmo
{
    namespace core
    {
        namespace resource
        {
            template <typename T> inline std::unique_ptr<Resource<T>> createLoader()
            {
                spdlog::warn("No Resource Loader registered for this type");
                return nullptr;
            }

            template <> inline std::unique_ptr<Resource<TTF_Font>> createLoader()
            {
                return std::make_unique<FontLoader>();
            }

            template <> inline std::unique_ptr<Resource<Bytecode>> createLoader()
            {
                return std::make_unique<ScriptLoader>();
            }

            template <> inline std::unique_ptr<Resource<SDL_Surface>> createLoader()
            {
                return std::make_unique<SurfaceLoader>();
            }

            template <> inline std::unique_ptr<Resource<SDL_Texture>> createLoader()
            {
                return std::make_unique<TextureLoader>();
            }
        } // namespace resource
    } // namespace core
} // namespace atmo
