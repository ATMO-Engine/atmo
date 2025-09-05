add_rules("mode.debug", "mode.release")

set_languages("c++23")

if is_mode("debug") then
    set_policy("build.sanitizer.address", true)
    set_policy("build.sanitizer.undefined", true)
end

if is_mode("release") then
    set_optimize("fastest")
end

set_config("build.compdb", true)
add_rules("plugin.compile_commands.autoupdate")

local SUBMODULE_PATH = "submodules/"
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
on_install(function(package)
    local configs = {"-DBUILD_STATIC_LIBS=ON", "-DFLECS_CPP_NO_AUTO_REGISTRATION=1"}
    table.insert(configs, "-DCMAKE_BUILD_TYPE=" .. (package:debug() and "Debug" or "Release"))
    import("package.tools.cmake").install(package, configs)
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

add_requires(
    "spdlog", { system = false },
    "luau", { system = false },
    "flecs", { system = false },
    "glaze", { system = false }
)

target("atmo")
    set_kind("binary")
    add_packages("spdlog", "luau", "flecs", "glaze")
    add_files("src/**.cpp")
    add_includedirs("src")

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
