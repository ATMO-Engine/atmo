add_rules("mode.debug", "mode.release")

set_languages("c++23")

if is_mode("debug") then
    set_policy("build.sanitizer.address", true)
    set_policy("build.sanitizer.undefined", true)
end

if is_mode("release") then
    set_optimize("fastest")
end

target("atmo")
    set_kind("binary")
    add_files("src/**.cpp")

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
        "MetalKit",
        "QuartzCore",
        "UniformTypeIdentifiers"
    )
end

local SUBMODULE_PATH = "submodules/"
-- ensure submodules are cloned in the ./submodules directory
if not os.isdir("submodules") then
    os.exec("git submodule update --init --recursive")
end

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
