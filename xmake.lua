set_policy("check.auto_ignore_flags", false)

add_rules("mode.release", "mode.debug", "mode.profile")
set_allowedmodes("release", "debug", "profile")

set_languages("c++23")

option("tsan")
    set_default(false)
    set_showmenu(true)
    set_description("Build debug mode with ThreadSanitizer instead of AddressSanitizer and UBSan")
option_end()

if is_mode("debug") then
    set_symbols("debug")
    set_optimize("none")

    if not is_plat("windows") then
        if has_config("tsan") then
            set_policy("build.sanitizer.thread", true)
        else
            set_policy("build.sanitizer.address", true)
            set_policy("build.sanitizer.undefined", true)
        end
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

option("rendering_vulkan")
    set_showmenu(true)
    set_description("Build the Vulkan RenderingPlatform (default: off on macOS, on elsewhere)")
    on_check(function (option)
        option:enable(not is_plat("macosx"))
    end)
option_end()

local VULKAN_SDK_VERSION = "1.4.335+0"

add_requires("slang v2026.20", { system = false, configs = { shared = false, asan = false, slangc = true, dxil = false, slang_llvm_flavor = "DISABLE" } })
if has_config("rendering_vulkan") then
    add_requires("vulkan-headers " .. VULKAN_SDK_VERSION, { system = false })
    add_requires("volk " .. VULKAN_SDK_VERSION, { system = false })
    add_requires("vulkan-memory-allocator v3.4.0", { system = false })
    add_requireconfs("volk.vulkan-headers", "vulkan-memory-allocator.vulkan-headers", { override = true, version = VULKAN_SDK_VERSION, system = false })
    if is_plat("macosx") then
        add_requires("moltenvk v1.4.2", { system = false, configs = { asan = false } })
    end
end

rule("slang.builtin")
    set_extensions(".slang")
    on_load(function (target)
        target:add("includedirs", path.join(target:autogendir(), "rules", "slang"))
    end)
    on_preparecmd_file(function (target, batchcmds, sourcefile, opt)
        import("core.project.project")

        local outdir = path.join(target:autogendir(), "rules", "slang", "shaders")
        local sourcedir = path.directory(sourcefile)
        local outputs = {}

        batchcmds:mkdir(outdir)

        if target:extraconf("rules", "slang.builtin", "embed_sources") then
            local header = path.join(outdir, path.filename(sourcefile) .. ".h")
            batchcmds:show_progress(opt.progress, "${color.build.object}embedding.slang %s", sourcefile)
            batchcmds:vlua("cli.binutils.bin2c", {"-i", path(sourcefile), "-o", path(header), "--nozeroend"})
            table.insert(outputs, header)
        end

        if io.readfile(sourcefile):find("%[shader%(") then
            local slangc = path.join(project.required_package("slang"):installdir(), "bin", is_host("windows") and "slangc.exe" or "slangc")
            for _, shader_target in ipairs(target:extraconf("rules", "slang.builtin", "targets") or {}) do
                local blob = path.join(outdir, path.basename(sourcefile) .. "." .. shader_target)
                batchcmds:show_progress(opt.progress, "${color.build.object}compiling.slang %s (%s)", sourcefile, shader_target)
                batchcmds:vrunv(slangc, {path(sourcefile), "-I", path(sourcedir), "-target", shader_target, "-O0", "-fvk-use-entrypoint-name", "-o", path(blob)})
                batchcmds:vlua("cli.binutils.bin2c", {"-i", path(blob), "-o", path(blob .. ".h"), "--nozeroend"})
                table.insert(outputs, blob .. ".h")
            end
        end

        if #outputs == 0 then
            return
        end

        batchcmds:add_depfiles(os.files(path.join(sourcedir, "*.slang")))
        batchcmds:set_depmtime(os.mtime(outputs[1]))
        batchcmds:set_depcache(target:dependfile(outputs[1]))
    end)
rule_end()

rule("moltenvk.bundle")
    after_build(function (target)
        import("core.project.project")

        local moltenvk = project.required_package("moltenvk")
        if moltenvk then
            os.cp(path.join(moltenvk:installdir(), "lib", "libMoltenVK.dylib"), target:targetdir())
        end
    end)
rule_end()

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

function rendering(opt)
    local shader_targets = {}
    if has_config("rendering_vulkan") then
        table.insert(shader_targets, "spirv")
        add_packages("vulkan-headers", "volk", "vulkan-memory-allocator")
        add_defines("ATMO_RENDERING_VULKAN")
        if is_plat("macosx") then
            add_rules("moltenvk.bundle")
        end
    else
        remove_files("src/server/rendering/api/vulkan/**.cpp")
    end

    add_rules("slang.builtin", { targets = shader_targets, embed_sources = opt.shader_compiler })
    add_files("src/server/rendering/shaders/*.slang")

    if opt.shader_compiler then
        add_packages("slang")
    else
        remove_files("src/server/rendering/shader/shader_compiler.cpp")
    end
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
    rendering({ shader_compiler = true })
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
    rendering({ shader_compiler = true })
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
    rendering({ shader_compiler = false })
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
