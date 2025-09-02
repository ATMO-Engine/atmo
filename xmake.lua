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
