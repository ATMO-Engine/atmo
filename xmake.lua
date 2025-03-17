add_rules("mode.debug", "mode.release")

local SUBMODULE_PATH = "submodules/"

package("glaze")
add_deps("cmake")
set_sourcedir(path.join(os.scriptdir(), SUBMODULE_PATH .. "glaze"))
on_install(function(package)
    local configs = {}
    table.insert(configs, "-DCMAKE_BUILD_TYPE=" .. (package:debug() and "Debug" or "Release"))
    table.insert(configs, "-DBUILD_STATIC_LIBS=" .. (package:config("static") and "ON" or "OFF"))
    table.insert(configs, "-DGLAZE_BUILD_EXAMPLES=OFF")
    table.insert(configs, "-DGLAZE_BUILD_TESTS=OFF")
    table.insert(configs, "-DGLAZE_BUILD_DOCS=OFF")
    -- table.insert(configs, "-DGLAZE_BUILD_TOOLS=OFF")
    table.insert(configs, "-Dglaze_DEVELOPER_MODE=OFF")
    table.insert(configs, "-DCMAKE_CXX_STANDARD=23")
    import("package.tools.cmake").install(package, configs)
end)
package_end()

package("flecs")
add_deps("cmake")
set_sourcedir(path.join(os.scriptdir(), SUBMODULE_PATH .. "flecs"))
on_install(function(package)
    local configs = {}
    table.insert(configs, "-DCMAKE_BUILD_TYPE=" .. (package:debug() and "Debug" or "Release"))
    table.insert(configs, "-DBUILD_STATIC_LIBS=" .. (package:config("static") and "ON" or "OFF"))
    table.insert(configs, "-DFLECS_CPP_NO_AUTO_REGISTRATION=1")
    import("package.tools.cmake").install(package, configs)
end)
package_end()

package("spdlog")
add_deps("cmake")
set_sourcedir(path.join(os.scriptdir(), SUBMODULE_PATH .. "spdlog"))
on_install(function(package)
    local configs = {}
    table.insert(configs, "-DCMAKE_BUILD_TYPE=" .. (package:debug() and "Debug" or "Release"))
    table.insert(configs, "-DBUILD_STATIC_LIBS=" .. (package:config("static") and "ON" or "OFF"))
    import("package.tools.cmake").install(package, configs)
end)
package_end()

-- add_requires("sdl3", "glaze", "clay", "imgui", "flecs", "luau", "curlpp", "joltphysics")


if is_mode("debug") then
    set_policy("build.sanitizer.address", true)
    set_policy("build.sanitizer.undefined", true)
end

add_requires(
    "libsdl3 release-3.2.8", { system = false },
    "libsdl3_image", { system = false },
    "glaze", { system = false },
    "flecs", { system = false },
    "spdlog", { system = false }
)

target("atmo")
set_languages("c++23")
set_kind("binary")
add_packages("libsdl3", "libsdl3_image", "glaze", "flecs", "spdlog", "imgui-paint")
add_files("src/**.cpp")
--add_defines("FLECS_CPP_NO_AUTO_REGISTRATION=1")

if is_mode("release") then
    set_optimize("fastest")
end

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
