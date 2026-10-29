package("metal-cpp")
    set_kind("library", { headeronly = true })
    set_homepage("https://developer.apple.com/metal/cpp/")
    set_description("Metal-cpp is a low-overhead C++ interface for Metal that helps you add Metal functionality to graphics apps, games, and game engines that are written in C++.")
    set_license("Apache-2.0")

    set_urls("https://developer.apple.com/metal/cpp/files/metal-cpp_$(version).zip",
         {version = function (version)
            local versions = {
                ["15.2"] = "macOS15.2_iOS18.2",
                ["26"]   = "26",
            }
            return versions[tostring(version)]
        end})

    add_versions("15.2", "3437e4abfbd3d45217f34772ef3502f31ba3358e5fb6ac9d0ca952a047bcfe25")
    add_versions("26",   "4df3c078b9aadcb516212e9cb03004cbc5ce9a3e9c068fa3144d021db585a3a4")

    add_frameworks("Foundation", "Metal", "QuartzCore")

    on_check(function (package)
        assert(package:is_plat("macosx", "iphoneos"), "package(metal-cpp) only support macosx or iOS")
    end)

    on_install("macosx", "iphoneos", function (package)
        local sourcedir = os.isdir("metal-cpp") and "metal-cpp" or "."
        for _, dir in ipairs({"Foundation", "Metal", "QuartzCore"}) do
            os.vcp(path.join(sourcedir, dir), package:installdir("include"))
        end
    end)

    on_test("macosx", "iphoneos", function (package)
        assert(package:check_cxxsnippets({test = [[
            #include <Foundation/Foundation.hpp>
            #include <Metal/Metal.hpp>
            #include <QuartzCore/QuartzCore.hpp>

            void test (MTL::Device* device, CA::MetalLayer* layer) {
                (void)device;
                (void)layer;
            }
        ]]}, {configs = {languages = "c++17"}}))
    end)
