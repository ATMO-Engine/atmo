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

set_policy("build.warning", true)

if is_mode("release") then
    set_optimize("fastest")
end

if is_mode("profile") then
    set_optimize("fastest")
end

set_config("build.compdb", true)
add_rules("plugin.compile_commands.autoupdate", {
    arguments = {"--target=" .. (get_config("target") or "all")}
})

local SUBMODULE_PATH = "submodules/"

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

package("luau")
    add_deps("cmake")
    set_sourcedir(path.join(os.scriptdir(), SUBMODULE_PATH .. "luau"))
    on_install(function(package)
        local configs = {"-DLUAU_BUILD_TESTS=OFF", "-DCMAKE_POLICY_DEFAULT_CMP0057=NEW", "-DBUILD_SHARED_LIBS=OFF", "-DLUAU_BUILD_WEB=OFF", "-DLUAU_EXTERN_C=OFF"}
        table.insert(configs, "-DCMAKE_BUILD_TYPE=" .. (package:is_debug() and "Debug" or "RelWithDebInfo"))
        import("package.tools.cmake").build(package, configs, { builddir = "build" })

        local cmake_file = io.readfile("CMakeLists.txt")

        local links = {}
        for library_name, library_type in cmake_file:gmatch("add_library%(([%a|%.]+) (%w+)") do
            library_type = library_type:lower()
            if library_name:startswith("Luau.") and (library_type == "static" or library_type == "interface") then
                if library_name:endswith(".lib") then
                    library_name = library_name:sub(1, -5)
                end
                if library_type == "static" then
                    table.insert(links, library_name)
                end
                local include_dir = library_name:sub(6)
                include_dir = include_dir:gsub("%..*", "")
                os.trycp(include_dir .. "/include/*", package:installdir("include"))
            end
        end

        for i = #links, 1, -1 do
            local link = links[i]
            package:add("links", link)
        end

        os.trycp("build/**.a", package:installdir("lib"))
        os.trycp("build/**.so", package:installdir("lib"))
        os.trycp("build/**.dylib", package:installdir("lib"))
        os.trycp("build/**.lib", package:installdir("lib"))
        os.trycp("build/**.dll", package:installdir("bin"))
        os.trycp("build/luau*", package:installdir("bin"))

        package:addenv("PATH", "bin")
    end)
package_end()

package("flecs")
add_deps("cmake")
set_sourcedir(path.join(os.scriptdir(), SUBMODULE_PATH .. "flecs"))

if is_plat("windows", "mingw") then
    add_syslinks("wsock32", "ws2_32")
elseif is_plat("linux") then
    add_syslinks("pthread")
elseif is_plat("bsd") then
    add_syslinks("execinfo", "pthread")
end

on_load("windows", "mingw", function (package)
    if not package:config("shared") then
        package:add("defines", "flecs_STATIC")
    end
end)

on_install(function(package)
    local configs = {}
    table.insert(configs, "-DCMAKE_BUILD_TYPE=" .. (package:debug() and "Debug" or "Release"))
    table.insert(configs, "-DFLECS_STATIC=ON")
    table.insert(configs, "-DFLECS_SHARED=OFF")
    table.insert(configs, "-DFLECS_PIC=OFF")
    table.insert(configs, "-DCMAKE_BUILD_TYPE=" .. (package:debug() and "Debug" or "Release"))
    table.insert(configs, "-DFLECS_CPP_NO_AUTO_REGISTRATION=1")
    table.insert(configs, "-DFLECS_META=ON")
    import("package.tools.cmake").install(package, configs)

    local pdb = path.join(package:builddir(), "flecs.pdb")
    os.trycp(pdb, package:installdir("lib"))
end)
package_end()

package("glaze")
add_deps("cmake")
set_sourcedir(path.join(os.scriptdir(), SUBMODULE_PATH .. "glaze"))
on_install(function(package)
    local configs = {"-DBUILD_STATIC_LIBS=ON"}
    table.insert(configs, "-DCMAKE_BUILD_TYPE=" .. (package:debug() and "Debug" or "Release"))
    table.insert(configs, "-DGLAZE_BUILD_EXAMPLES=OFF")
    table.insert(configs, "-DGLAZE_BUILD_TESTS=OFF")
    table.insert(configs, "-DGLAZE_BUILD_DOCS=OFF")
    -- table.insert(configs, "-DGLAZE_BUILD_TOOLS=OFF")
    table.insert(configs, "-Dglaze_DEVELOPER_MODE=OFF")
    table.insert(configs, "-DCMAKE_CXX_STANDARD=23")
    import("package.tools.cmake").install(package, configs)
end)
package_end()

package("libsdl3")
add_deps("cmake")
set_sourcedir(path.join(os.scriptdir(), SUBMODULE_PATH .. "sdl"))
on_install(function(package)
    local configs = {"-DBUILD_STATIC_LIBS=ON"}
    table.insert(configs, "-DCMAKE_BUILD_TYPE=" .. (package:debug() and "Debug" or "Release"))
    table.insert(configs, "-DCMAKE_CXX_STANDARD=23")
    table.insert(configs, "-DSDL_TEST=OFF")
    import("package.tools.cmake").install(package, configs)
end)
package_end()

package("libsdl3_ttf")
add_deps("cmake", "libsdl3", "freetype")
set_sourcedir(path.join(os.scriptdir(), SUBMODULE_PATH .. "sdl_ttf"))
on_install(function (package)
        local configs = {"-DSDLTTF_SAMPLES=OFF", "-DSDLTTF_VENDORED=OFF"}
        table.insert(configs, "-DCMAKE_BUILD_TYPE=" .. (package:is_debug() and "Debug" or "Release"))
        table.insert(configs, "-DBUILD_SHARED_LIBS=" .. (package:config("shared") and "ON" or "OFF"))
        table.insert(configs, "-DSDLTTF_HARFBUZZ=" .. (package:config("harfbuzz") and "ON" or "OFF"))
        table.insert(configs, "-DSDLTTF_PLUTOSVG=" .. (package:config("plutosvg") and "ON" or "OFF"))
        local freetype = package:dep("freetype")
        if freetype then
            local fetchinfo = freetype:fetch()
            if fetchinfo then
                local includedirs = table.wrap(fetchinfo.includedirs or fetchinfo.sysincludedirs)
                if #includedirs > 0 then
                    table.insert(configs, "-DFREETYPE_INCLUDE_DIRS=" .. table.concat(includedirs, ";"))
                end
                local libfiles = table.wrap(fetchinfo.libfiles)
                if #libfiles > 0 then
                    table.insert(configs, "-DFREETYPE_LIBRARY=" .. libfiles[1])
                end
                if not freetype:config("shared") then
                    local libfiles = {}
                    for _, dep in ipairs(freetype:librarydeps()) do
                        local depinfo = dep:fetch()
                        if depinfo then
                            table.join2(libfiles, depinfo.libfiles)
                        end
                    end
                    if #libfiles > 0 then
                        local libraries = ""
                        for _, libfile in ipairs(libfiles) do
                            libraries = libraries .. " " .. (libfile:gsub("\\", "/"))
                        end
                        io.replace("CMakeLists.txt", "target_link_libraries(${sdl3_ttf_target_name} PRIVATE Freetype::Freetype)",
                            "target_link_libraries(${sdl3_ttf_target_name} PRIVATE Freetype::Freetype " .. libraries .. ")", {plain = true})
                    end
                end
            end
        end
        import("package.tools.cmake").install(package, configs, {packagedeps={"plutovg"}})
    end)
package_end()

package("libsdl3_image")
add_deps("cmake", "libsdl3")
set_sourcedir(path.join(os.scriptdir(), SUBMODULE_PATH .. "sdl_image"))
on_install(function(package)
    local configs = {"-DBUILD_STATIC_LIBS=ON", "-DSDLIMAGE_SAMPLES=OFF", "-DSDLIMAGE_TESTS=OFF", "-DSDLIMAGE_VENDORED=OFF"}
    table.insert(configs, "-DCMAKE_BUILD_TYPE=" .. (package:debug() and "Debug" or "Release"))
    table.insert(configs, "-DCMAKE_CXX_STANDARD=23")
    import("package.tools.cmake").install(package, configs)
end)
package_end()

package("clay")
set_sourcedir(path.join(os.scriptdir(), SUBMODULE_PATH .. "clay"))
on_install(function(package)
    os.cp("clay.h", package:installdir("include"))
end)
package_end()

package("catch2")
    add_deps("cmake")
    set_sourcedir(path.join(os.scriptdir(), SUBMODULE_PATH .. "catch2"))
    on_install(function(package)
        local configs = {"-DBUILD_STATIC_LIBS=ON", "-DBUILD_TESTING=OFF"}
        table.insert(configs, "-DCMAKE_BUILD_TYPE=" .. (package:debug() and "Debug" or "Release"))
        import("package.tools.cmake").install(package, configs)
    end)
package_end()

package("semver")
    add_deps("cmake")
    set_sourcedir(path.join(os.scriptdir(), SUBMODULE_PATH .. "semver"))
    on_install(function(package)
        os.cp("include/semver.hpp", package:installdir("include"))
    end)
package_end()

package("tracy")
    set_sourcedir(path.join(os.scriptdir(), SUBMODULE_PATH .. "tracy"))
    on_install(function (package)
        os.cp("public/*", package:installdir("include"))
    end)
package_end()

package("box2d")

    if is_plat("linux", "bsd") then
        add_syslinks("pthread")
    end

    add_deps("cmake")
    set_sourcedir(path.join(os.scriptdir(), SUBMODULE_PATH .. "box2d"))

    on_install("!bsd", function (package)
        if package:is_plat("windows") and package:is_debug() then
            package:add("defines", "B2_ENABLE_ASSERT")
        end

        io.replace("CMakeLists.txt", [[set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")]], "", {plain = true})

        local configs = {
            "-DBOX2D_BUILD_UNIT_TESTS=OFF",
            "-DBOX2D_BUILD_TESTBED=OFF",

            "-DBOX2D_SAMPLES=OFF",
            "-DBOX2D_UNIT_TESTS=OFF",
            "-DBOX2D_VALIDATE=OFF",
            "-DBUILD_SHARED_LIBS=OFF",
            "--compile-no-warning-as-error",
        }
        table.insert(configs, "-DCMAKE_BUILD_TYPE=" .. (package:is_debug() and "Debug" or "Release"))
        table.insert(configs, "-DBOX2D_SANITIZE=" .. (package:config("asan") and "ON" or "OFF"))
        table.insert(configs, "-DBOX2D_AVX2=OFF")

        os.mkdir(path.join(package:builddir(), "src/pdb"))
        import("package.tools.cmake").install(package, configs)
    end)
package_end()

add_requires(
    "spdlog", { system = false },
    "luau", { system = false },
    "flecs", {configs = {shared = false}, system = false},
    "glaze", { system = false },
    "libsdl3", { system = false },
    "libsdl3_ttf", { system = false },
    "libsdl3_image", { system = false },
    "clay", { system = false },
    "catch2", { system = false },
    "semver", { system = false },
    "tracy", { system = false },
    "box2d", { system = false }
)

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

    if is_plat("windows") then
        add_cxxflags("/utf-8")
        add_syslinks(
            "User32", "Gdi32", "Ole32", "Advapi32", "Shell32", "Comdlg32", "Setupapi",
            "Winmm", "OleAut32", "Version", "Imm32"
        )
    end
end

function packages()
    add_packages("spdlog", "luau", "flecs", "glaze", "libsdl3", "libsdl3_ttf", "libsdl3_image", "clay", "semver", "tracy", "box2d")
end

target("atmo")
    set_kind("binary")
    set_default(true)
    packages()
    add_files("src/**.cpp")
    add_includedirs("src")
    platform_specifics()
    if is_mode("debug") or is_mode("profile") then
        add_defines("ATMO_PROFILING")
        add_defines("ATMO_PROFILER_TRACY")
        add_defines("TRACY_ENABLE")
        add_files(path.join(os.scriptdir(), SUBMODULE_PATH .. "tracy/public/TracyClient.cpp"))
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
        -- normalize to forward slashes
        for i, f in ipairs(files) do
            files[i] = path.unix(f)
        end

        print(bin .. ": Packing " .. #files .. " files...")

        os.runv(bin, table.join({"--pack"}, files))

        local pck = path.absolute("packed_output.pck")

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
    platform_specifics()
    add_tests("atmo-test", {
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

        if not table.contains({"soft", "full", "submodules", "all"}, mode) then
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
                    " - submodules",
                    " - all"
            }
        }
    })
task_end()
