local function clean_soft()
    import("core.project.config")

    config.load()

    local buildir = config.get("builddir")

    os.tryrm(buildir.."/.deps/*/*/*/**")
    os.tryrm(buildir.."/.objs/*/*/*/**")
    os.tryrm(buildir.."/$(host)/*/*/*")
end

local function clean_full()
    clean_soft()
    os.tryrm(".xmake")
    os.tryrm("compile_commands.json")
    os.tryrm("build")
    os.tryrm(".cache")
end

local function clean_packages()
    os.execv(os.programfile(), {"require", "--uninstall", "-y"})
end

local function clean_all()
    clean_packages()
    clean_full()
end

local modes = {
    {"soft", clean_soft},
    {"full", clean_full},
    {"all", clean_all}
}

function main(mode)
    for _, m in ipairs(modes) do
        if mode == m[1] then
            m[2]()
            return
        end
    end
end
