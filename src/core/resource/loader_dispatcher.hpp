#include <memory>
#include <string>
#include "core/resource/loaders/image_loader.hpp"
#include "core/resource/loaders/script_loader.hpp"
#include "core/resource/resource.hpp"
#include "core/resource/loaders/font_loader.hpp"


namespace atmo {
    namespace core {
        namespace resource {
            template<typename T>
            std::unique_ptr<Resource<T>> createLoader();

            template<>
            std::unique_ptr<Resource<TTF_Font>> createLoader()
            {
                return std::make_unique<FontLoader>();
            }

            template<>
            std::unique_ptr<Resource<Bytecode>> createLoader()
            {
                return std::make_unique<ScriptLoader>();
            }

            template<>
            std::unique_ptr<Resource<std::string>> createLoader()
            {
                return std::make_unique<ImageLoader>();
            }
        } // namespace resource
    } // namespace core
} // namespace atmo
