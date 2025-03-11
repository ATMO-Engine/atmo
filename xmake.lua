add_rules("mode.debug", "mode.release")

local SUBMODULE_PATH = "submodules/"

package("libsdl3")
    add_deps("cmake")
    set_sourcedir(path.join(os.scriptdir(), SUBMODULE_PATH .. "SDL"))
    on_install(function (package)
        local configs = {}
        table.insert(configs, "-DCMAKE_BUILD_TYPE=" .. (package:debug() and "Debug" or "Release"))
        table.insert(configs, "-DBUILD_SHARED_LIBS=" .. (package:config("shared") and "ON" or "OFF"))
        import("package.tools.cmake").install(package, configs)
    end)
package_end()

package("simdjson")
    add_deps("cmake")
    set_sourcedir(path.join(os.scriptdir(), SUBMODULE_PATH .. "simdjson"))
    on_install(function (package)
        local configs = {}
        table.insert(configs, "-DCMAKE_BUILD_TYPE=" .. (package:debug() and "Debug" or "Release"))
        table.insert(configs, "-DBUILD_SHARED_LIBS=" .. (package:config("shared") and "ON" or "OFF"))
        import("package.tools.cmake").install(package, configs)
    end)
package_end()

package("flecs")
    add_deps("cmake")
    set_sourcedir(path.join(os.scriptdir(), SUBMODULE_PATH .. "flecs"))
    on_install(function (package)
        local configs = {}
        table.insert(configs, "-DCMAKE_BUILD_TYPE=" .. (package:debug() and "Debug" or "Release"))
        table.insert(configs, "-DBUILD_SHARED_LIBS=" .. (package:config("shared") and "ON" or "OFF"))
        import("package.tools.cmake").install(package, configs)
    end)
package_end()

package("spdlog")
    add_deps("cmake")
    set_sourcedir(path.join(os.scriptdir(), SUBMODULE_PATH .. "spdlog"))
    on_install(function (package)
        local configs = {}
        table.insert(configs, "-DCMAKE_BUILD_TYPE=" .. (package:debug() and "Debug" or "Release"))
        table.insert(configs, "-DBUILD_SHARED_LIBS=" .. (package:config("shared") and "ON" or "OFF"))
        import("package.tools.cmake").install(package, configs)
    end)
package_end()

-- add_requires("sdl3", "simdjson", "clay", "imgui", "flecs", "luau", "curlpp", "joltphysics")
add_requires(
    "libsdl3", {system = false},
    "simdjson", {system = false},
    "flecs", {system = false},
    "spdlog", {system = false}
)

target("atmo")
    set_warnings("all", "error")
    set_languages("c++20")
    set_kind("binary")
    add_packages("libsdl3", "simdjson", "flecs", "spdlog")
    add_files("src/*.cpp")

    -- ImGui
    add_includedirs(SUBMODULE_PATH .. "imgui")
    add_files(SUBMODULE_PATH .. "imgui/*.cpp")
    add_files(SUBMODULE_PATH .. "imgui/backends/imgui_impl_sdl3.cpp")
    add_includedirs(SUBMODULE_PATH .. "imgui/backends")

    if is_plat("macosx") then
        add_frameworks(
            "AppKit",
            "AVFoundation",
            "AudioToolbox",
            "Carbon",
            "Cocoa",
            "CoreAudio",
            "CoreFoundation",
            "CoreGraphics",
            "CoreHaptics",
            "CoreMedia",
            "CoreServices",
            "CoreVideo",
            "ForceFeedback",
            "GameController",
            "IOKit",
            "Metal",
            "Metal",
            "MetalKit",
            "QuartzCore",
            "UniformTypeIdentifiers"
        )
    end

--
-- If you want to known more usage about xmake, please see https://xmake.io
--
-- ## FAQ
--
-- You can enter the project directory firstly before building project.
--
--   $ cd projectdir
--
-- 1. How to build project?
--
--   $ xmake
--
-- 2. How to configure project?
--
--   $ xmake f -p [macosx|linux|iphoneos ..] -a [x86_64|i386|arm64 ..] -m [debug|release]
--
-- 3. Where is the build output directory?
--
--   The default output directory is `./build` and you can configure the output directory.
--
--   $ xmake f -o outputdir
--   $ xmake
--
-- 4. How to run and debug target after building project?
--
--   $ xmake run [targetname]
--   $ xmake run -d [targetname]
--
-- 5. How to install target to the system directory or other output directory?
--
--   $ xmake install
--   $ xmake install -o installdir
--
-- 6. Add some frequently-used compilation flags in xmake.lua
--
-- @code
--    -- add debug and release modes
--    add_rules("mode.debug", "mode.release")
--
--    -- add macro definition
--    add_defines("NDEBUG", "_GNU_SOURCE=1")
--
--    -- set warning all as error
--    set_warnings("all", "error")
--
--    -- set language: c99, c++11
--    set_languages("c99", "c++11")
--
--    -- set optimization: none, faster, fastest, smallest
--    set_optimize("fastest")
--
--    -- add include search directories
--    add_includedirs("/usr/include", "/usr/local/include")
--
--    -- add link libraries and search directories
--    add_links("tbox")
--    add_linkdirs("/usr/local/lib", "/usr/lib")
--
--    -- add system link libraries
--    add_syslinks("z", "pthread")
--
--    -- add compilation and link flags
--    add_cxflags("-stdnolib", "-fno-strict-aliasing")
--    add_ldflags("-L/usr/local/lib", "-lpthread", {force = true})
--
-- @endcode
--

