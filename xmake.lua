set_policy("check.auto_ignore_flags", false)

add_rules("mode.release", "mode.debug", "mode.profile")
set_allowedmodes("release", "debug", "profile")

set_languages("c++23")

if is_mode("debug") then
    set_symbols("debug")
    set_optimize("none")

    if not is_plat("windows") then
        set_policy("build.sanitizer.address", true)
        set_policy("build.sanitizer.undefined", true)
    end
end

set_policy("build.intermediate_directory", false)

set_policy("build.warning", true)

if is_mode("release") then
    set_optimize("fastest")
end

if is_mode("profile") then
    set_optimize("fastest")
end

set_config("build.compdb", true)
add_rules("plugin.compile_commands.autoupdate", {
    arguments = {"--target=" .. (get_config("target") or "atmo")}
})

set_policy("package.requires_lock", true)
add_repositories("atmo-deps deps")

local SDL_VERSION = "3.4.12"

add_requires("spdlog v1.15.3", { system = false })
add_requires("luau 0.689", { system = false })
add_requires("flecs v4.1.4", { system = false })
add_requires("glaze v7.2.2", { system = false })
add_requires("libsdl3 " .. SDL_VERSION, { system = false })
add_requires("libsdl3_ttf 2025.09.12", { system = false })
add_requires("libsdl3_image 3.4.0", { system = false })
add_requires("clay 2025.08.19", { system = false })
add_requires("catch2 v3.10.0", { system = false })
add_requires("box2d v3.1.1", { system = false })
add_requires("argparse v3.2", { system = false })
add_requires("ctre v3.11.0", { system = false })
if is_mode("debug", "profile") then
    add_requires("tracy v0.13.1", { system = false, configs = { enforce_callstack = false } })
end

add_requireconfs("libsdl3_ttf.libsdl3", "libsdl3_image.libsdl3", { override = true, version = SDL_VERSION, system = false })

function platform_specifics()
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

    if is_plat("linux") then
        add_syslinks("dl")
        -- lets dlopen()'d addons resolve engine symbols against this binary
        add_ldflags("-rdynamic", {force = true})
    end

    if is_plat("windows") then
        add_cxxflags("/utf-8")
        add_syslinks(
            "User32", "Gdi32", "Ole32", "Advapi32", "Shell32", "Comdlg32", "Setupapi",
            "Winmm", "OleAut32", "Version", "Imm32"
        )
    end
end

function packages()
    add_packages("spdlog", "luau", "flecs", "glaze", "libsdl3", "libsdl3_ttf", "libsdl3_image", "clay", "box2d", "argparse", "ctre")
    add_defines("FLECS_CPP_NO_AUTO_REGISTRATION", "FLECS_META=")
end

target("atmo")
    set_kind("binary")
    set_default(true)
    packages()
    add_files("src/**.cpp")
    add_includedirs("src")
    if is_plat("macosx") then
        add_files("src/editor/menu_bar/macos_menu_bar.mm")
    end
    platform_specifics()
    if is_mode("debug") or is_mode("profile") then
        add_defines("ATMO_PROFILING")
        add_defines("ATMO_PROFILER_TRACY")
        add_defines("TRACY_ENABLE")
        add_packages("tracy")
    end

    if is_mode("debug") then
        add_defines("ATMO_DEBUG")
    end

    local packed_hash = nil

    before_build(function (target)
        local pck = path.absolute(target:targetfile())
        if os.isfile(pck) then
            local data = io.readfile(pck, {encoding = "binary"})
            packed_hash = hash.md5(path.absolute(target:targetfile()))
        else
            packed_hash = nil
        end
    end)

    after_build(function (target)
        local function append_file(dst, src)
            local fsrc = assert(io.open(src, "rb"))
            local fdst = assert(io.open(dst, "ab"))

            local data = fsrc:read("*all")
            fdst:write(data)

            fsrc:close()
            fdst:close()
        end

        if not packed_hash or packed_hash ~= hash.md5(path.absolute(target:targetfile())) then
            print(target:targetfile() .. ": Detected changes, repacking assets...")
        else
            print(target:targetfile() .. ": No changes detected, skipping repack.")
            return
        end

        local bin = path.absolute(target:targetfile())

        local files = {}
        table.join2(files, os.files("translation/**"))
        table.join2(files, os.files("assets/**"))

        for i, f in ipairs(files) do
            files[i] = path.unix(f)
        end

        print(target:targetfile() .. ": Packing " .. #files .. " files...")

        os.runv(bin, table.join({"--headless", "--pack"}, files))

        local pck = path.absolute("packed_output.pck")

        print(target:targetfile() .. ": Packed output size is " .. string.format("%.2f mb", os.filesize(pck) / 1000000))

        append_file(bin, pck)

        os.rm(pck)
    end)
target_end()

target("atmo-test")
    set_kind("binary")
    set_default(false)

    if is_plat("windows") then
        add_ldflags("/SUBSYSTEM:CONSOLE", {force = true})
    end

    add_deps("atmo")
    packages()
    add_packages("catch2")
    add_files("tests/**.cpp")
    add_files("src/*/**.cpp")
    add_includedirs("src")
    if is_plat("macosx") then
        add_files("src/editor/menu_bar/macos_menu_bar.mm")
    end
    platform_specifics()
    add_tests("atmo-test", {
        rundir = os.projectdir(),
        runargs = {
            "--reporter=JUnit::out=test_results.xml",
            "--reporter=console::out=-::colour-mode=ansi"
        }
    })
target_end()

target("atmo-export")
    set_kind("binary")
    set_default(false)
    packages()
    add_files("src/**.cpp|src/editor/**.cpp")
    add_includedirs("src")
    platform_specifics()
    add_defines("ATMO_EXPORT")
target_end()



task("clean")
    on_run(function ()
        import("core.base.option")
        local mode = option.get("mode")

        if not table.contains({"soft", "full", "all"}, mode) then
            raise("invalid clean mode: %s", mode)
        end

        import("clean")(mode)
    end)

    set_menu({
        usage = "xmake clean|c [mode]",

        shortname = "c",

        description = "Clean project files.",

        options = {
            {
                nil, "mode", "v", "soft", "Clean mode.",
                    " - soft",
                    " - full",
                    " - all"
            }
        }
    })
task_end()
