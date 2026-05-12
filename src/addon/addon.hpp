#pragma once

#include <filesystem>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "core/args/arg_manager.hpp"
#include "core/ecs/entity_registry.hpp"
#include "core/engine.hpp"
#include "core/input/input_manager.hpp"
#include "core/resource/subresource_registry.hpp"
#include "impl/romver.hpp"
#include "meta/meta_registry.hpp"
#include "project/project_settings.hpp"

#if defined(_WIN32)
#include <windows.h>
#else
#include <dlfcn.h>
#endif

#if defined(_WIN32)
#define _ATMO_LIBRARY_EXTENSION "dll"
#elif defined(__APPLE__)
#define _ATMO_LIBRARY_EXTENSION "dylib"
#else
#define _ATMO_LIBRARY_EXTENSION "so"
#endif

#if defined(_WIN32)
#define _ATMO_ADDON_EXPORT extern "C" __declspec(dllexport)
#else
#define _ATMO_ADDON_EXPORT extern "C" __attribute__((visibility("default")))
#endif
#define _ATMO_REGISTER_ADDON_FUNC void atmo_register_addon_entrypoint(const AtmoAPI *api)

#define ATMO_REGISTER_ADDON() _ATMO_ADDON_EXPORT _ATMO_REGISTER_ADDON_FUNC

namespace atmo::addon
{
    struct AddonHeader {
        /**
         * @brief Pretty name of the addon. Can contain any character.
         */
        std::string name;

        /**
         * @brief Internal name, used for folder name and binary library name. Can only contain valid file system characters.
         */
        std::string internal_name;

        /**
         * @brief Version of the addon.
         */
        impl::Romver version{ "0.0.1" };

        /**
         * @brief Version of atmo this addon is made for.
         */
        impl::Romver atmo_version{ ATMO_VERSION };

        /**
         * @brief Title of the legal license used.
         */
        std::string license_title;

        /**
         * @brief List of authors.
         */
        std::vector<std::string> authors;

        /**
         * @brief Main website/homepage.
         */
        std::string website;

        /**
         * @brief Source of this addon. Can be a git link or atmo API link.
         */
        std::string source_link;

        /**
         * @brief Indicates if the addon includes a binary library.
         */
        bool has_library;
    };

    struct AtmoAPI {
        core::Engine *engine;
        core::InputManager &input_manager;
        core::args::ArgManager &arg_manager;

        core::ecs::EntityRegistry &entity_registry;
        core::resource::SubResourceRegistry &subresource_registry;
        meta::MetaRegistry &meta_registry;
    };

    class Addon
    {
    public:
        /**
         * @brief Creates a new Addon object. Does not load any data or library, only initializes the object.
         */
        Addon() = default;

        /**
         * @brief Initialize an Addon from file system
         *
         * @param path Path to the Addon folder on the file system
         */
        Addon(std::string_view path, const AtmoAPI *const api);

        ~Addon();

        class AddonLoadException : public std::runtime_error
        {
        public:
            explicit AddonLoadException(const std::string &reason) : std::runtime_error(std::format("Failed to load addon: {}", reason)), m_reason(reason) {}

            const std::string &getReason() const
            {
                return m_reason;
            }

        private:
            std::string m_reason;
        };

        AddonHeader header;

    private:
        void loadHeader();
        void loadLibrary(const AtmoAPI *api);
        std::filesystem::path extractLibraryToTemp();

        std::string m_path;

#if defined(_WIN32)
        HMODULE m_lib_handle = nullptr;
#else
        void *m_lib_handle = nullptr;
#endif
    };
} // namespace atmo::addon
