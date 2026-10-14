package("clay")
    set_kind("library", {headeronly = true})
    set_homepage("https://nicbarker.com/clay")
    set_description("High performance UI layout library in C.")
    set_license("zlib")

    add_urls("https://github.com/nicbarker/clay.git")

    add_versions("2025.08.19", "1bc5105272884a745a200fee70305723ce6e8e69")

    on_install(function (package)
        os.cp("clay.h", package:installdir("include"))
    end)

    on_test(function (package)
        assert(package:has_cincludes("clay.h"))
    end)
package_end()
