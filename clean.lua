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

local function clean_submodules()
    local submodules = os.dirs("submodules/*")
    for _, submodule in ipairs(submodules) do
        if os.isdir(submodule.."/build") then
            os.tryrm(submodule.."/build")
        end
    end
end

local function clean_all()
    clean_submodules()
    clean_full()
end

local modes = {
    {"soft", clean_soft},
    {"full", clean_full},
    {"submodules", clean_submodules},
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
