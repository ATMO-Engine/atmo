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

package("glaze")
    add_deps("cmake")
    set_sourcedir(path.join(os.scriptdir(), SUBMODULE_PATH .. "glaze"))
    on_install(function (package)
        local configs = {}
        table.insert(configs, "-DCMAKE_BUILD_TYPE=" .. (package:debug() and "Debug" or "Release"))
        table.insert(configs, "-DBUILD_STATIC_LIBS=" .. (package:config("static") and "ON" or "OFF"))
        import("package.tools.cmake").install(package, configs)
    end)
package_end()

package("flecs")
    add_deps("cmake")
    set_sourcedir(path.join(os.scriptdir(), SUBMODULE_PATH .. "flecs"))
    on_install(function (package)
        local configs = {}
        table.insert(configs, "-DCMAKE_BUILD_TYPE=" .. (package:debug() and "Debug" or "Release"))
        table.insert(configs, "-DBUILD_STATIC_LIBS=" .. (package:config("static") and "ON" or "OFF"))
        import("package.tools.cmake").install(package, configs)
    end)
package_end()

package("spdlog")
    add_deps("cmake")
    set_sourcedir(path.join(os.scriptdir(), SUBMODULE_PATH .. "spdlog"))
    on_install(function (package)
        local configs = {}
        table.insert(configs, "-DCMAKE_BUILD_TYPE=" .. (package:debug() and "Debug" or "Release"))
        table.insert(configs, "-DBUILD_STATIC_LIBS=" .. (package:config("static") and "ON" or "OFF"))
        import("package.tools.cmake").install(package, configs)
    end)
package_end()

-- add_requires("sdl3", "glaze", "clay", "imgui", "flecs", "luau", "curlpp", "joltphysics")
add_requires(
    "libsdl3", {system = false},
    "glaze", {system = false},
    "flecs", {system = false},
    "spdlog", {system = false}
)

target("atmo")
    set_warnings("all", "error")
    set_languages("c++23")
    set_kind("binary")
    add_packages("libsdl3", "glaze", "flecs", "spdlog")
    add_files("src/**.cpp")

    -- ImGui
    add_includedirs(SUBMODULE_PATH .. "imgui")
    add_files(SUBMODULE_PATH .. "imgui/*.cpp")
    add_files(SUBMODULE_PATH .. "imgui/backends/imgui_impl_sdl3.cpp")
    add_files(SUBMODULE_PATH .. "imgui/backends/imgui_impl_opengl3.cpp")
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
