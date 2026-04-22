#include "addon.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <vector>

#include "glaze/glaze.hpp"
#include "project/file.hpp"
#include "project/file_system.hpp"

#if defined(_WIN32)
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace atmo::addon
{
    Addon::Addon(std::string_view path, const AtmoAPI *api) : m_path(path)
    {
        std::vector<std::string> files = project::FileSystem::SearchFiles(path);

        if (std::find(files.begin(), files.end(), m_path + "/addon.json") == files.end()) {
            throw AddonLoadException("addon.json not found in addon directory.");
        }

        loadHeader();

        if (header.has_library)
            loadLibrary(api);
    }

    void Addon::loadHeader()
    {
        project::File file = project::FileSystem::OpenFile(m_path + "/addon.json");
        std::string file_contents = file.readAll();

        auto err = glz::read_json<AddonHeader>(header, file_contents);

        if (err) {
            throw AddonLoadException(std::format("Failed to parse addon header: {}", glz::format_error(err, file_contents)));
        }

        if (header.atmo_version != impl::Romver(ATMO_VERSION)) {
            throw AddonLoadException(std::format("ATMO version mismatch: expected {}, got {}", ATMO_VERSION, header.atmo_version.toString()));
        }
    }

    Addon::~Addon()
    {
        if (m_lib_handle) {
#if defined(_WIN32)
            FreeLibrary(m_lib_handle);
#else
            dlclose(m_lib_handle);
#endif
            m_lib_handle = nullptr;
        }
    }

    std::filesystem::path Addon::extractLibraryToTemp()
    {
        std::filesystem::path temp_dir = std::filesystem::temp_directory_path() / "atmo_addons" / header.internal_name / header.version.toString();

        std::filesystem::path temp_lib = temp_dir / std::format("{}.{}", header.internal_name, _ATMO_LIBRARY_EXTENSION);

        project::File lib_file =
            project::FileSystem::OpenFile(std::format("{}/{}.{}", m_path, header.internal_name, _ATMO_LIBRARY_EXTENSION), std::ios::in | std::ios::binary);

        if (std::filesystem::exists(temp_lib) && std::filesystem::file_size(temp_lib) == lib_file.size())
            return temp_lib;

        std::filesystem::create_directories(temp_dir);

        std::string bytes = lib_file.readAll();
        std::ofstream out(temp_lib, std::ios::binary | std::ios::trunc);
        if (!out.is_open())
            throw AddonLoadException(std::format("Failed to create temp file: {}", temp_lib.string()));

        out.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
        return temp_lib;
    }

    void Addon::loadLibrary(const AtmoAPI *const api)
    {
        std::filesystem::path lib_path;

        if (m_path.starts_with("project://")) {
            lib_path = extractLibraryToTemp();
        } else {
            lib_path = std::filesystem::path(m_path) / std::format("{}.{}", header.internal_name, _ATMO_LIBRARY_EXTENSION);
        }

#if defined(_WIN32)
        m_lib_handle = LoadLibraryW(lib_path.wstring().c_str());
        if (!m_lib_handle)
            throw AddonLoadException(std::format("LoadLibrary failed for '{}': error {}", lib_path.string(), GetLastError()));

        auto *entrypoint = reinterpret_cast<void (*)(const AtmoAPI *const api)>(GetProcAddress(m_lib_handle, "atmo_register_addon_entrypoint"));
        if (!entrypoint) {
            FreeLibrary(m_lib_handle);
            m_lib_handle = nullptr;
            throw AddonLoadException(std::format("Symbol 'atmo_register_addon_entrypoint' not found in '{}'", lib_path.string()));
        }
#else
        m_lib_handle = dlopen(lib_path.c_str(), RTLD_NOW | RTLD_LOCAL);
        if (!m_lib_handle)
            throw AddonLoadException(std::format("dlopen failed for '{}': {}", lib_path.string(), dlerror()));

        dlerror();
        auto *entrypoint = reinterpret_cast<void (*)(const AtmoAPI *const api)>(dlsym(m_lib_handle, "atmo_register_addon_entrypoint"));
        const char *dl_err = dlerror();
        if (dl_err) {
            dlclose(m_lib_handle);
            m_lib_handle = nullptr;
            throw AddonLoadException(std::format("dlsym failed for 'atmo_register_addon_entrypoint' in '{}': {}", lib_path.string(), dl_err));
        }
#endif

        entrypoint(api);
    }
} // namespace atmo::addon
