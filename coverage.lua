import("core.base.json")
import("core.project.config")
import("lib.detect.find_program")

local function outputdir()
    return path.absolute(path.join(config.builddir(), "coverage"))
end

local function sourcedir()
    return path.join(os.projectdir(), "src")
end

local function kind(target)
    if target:has_tool("cxx", "clang", "clangxx", "clang_cl") then
        return "llvm"
    elseif target:has_tool("cxx", "gcc", "gxx") then
        return "gcov"
    end
end

local function llvm_tool(name)
    if is_host("macosx") then
        local found = try { function () return os.iorunv("xcrun", {"--find", name}):trim() end }
        if found and #found > 0 then
            return found
        end
    end
    local envname = (name:upper():gsub("-", "_"))
    local found = find_program(os.getenv(envname) or name)
    if not found then
        raise("coverage: %s not found, install LLVM or set %s to its path", name, envname)
    end
    return found
end

local function print_summary(rows)
    cprint("${bright}coverage of src/:${clear}")
    for _, row in ipairs(rows) do
        if row.total > 0 then
            print(string.format("  %-9s %6.2f%%  (%d/%d)", row.name, row.covered * 100 / row.total, row.covered, row.total))
        end
    end
    print("  report:   " .. path.join(outputdir(), "html", "index.html"))
    print("  lcov:     " .. path.join(outputdir(), "lcov.info"))
end

function instrument(target)
    local tool = kind(target)
    if tool == "llvm" then
        local flags = {"-fprofile-instr-generate", "-fcoverage-mapping"}
        target:add("cxflags", flags, {force = true})
        target:add("mxflags", flags, {force = true})
        target:add("ldflags", "-fprofile-instr-generate", {force = true})
    elseif tool == "gcov" then
        target:add("cxflags", "--coverage", {force = true})
        target:add("ldflags", "--coverage", {force = true})
    else
        raise("coverage: %s needs clang or gcc", target:name())
    end
end

function prepare(target)
    local dir = outputdir()
    os.tryrm(dir)
    os.mkdir(dir)
    if kind(target) == "llvm" then
        os.setenv("LLVM_PROFILE_FILE", path.join(dir, "raw", target:name() .. "-%p.profraw"))
    else
        os.tryrm(path.join(target:objectdir(), "**.gcda"))
    end
end

local function report_llvm(target)
    local dir = outputdir()
    local rawfiles = os.files(path.join(dir, "raw", "*.profraw"))
    if #rawfiles == 0 then
        cprint("${color.warning}coverage: %s wrote no profile", target:name())
        return
    end

    local profdata = path.join(dir, target:name() .. ".profdata")
    os.vrunv(llvm_tool("llvm-profdata"), table.join({"merge", "-sparse", "-o", profdata}, rawfiles))

    local llvm_cov = llvm_tool("llvm-cov")
    local binary = path.absolute(target:targetfile())
    local common = {"-instr-profile=" .. profdata, binary, sourcedir()}

    os.vrunv(llvm_cov, table.join({"show", "-format=html", "-output-dir=" .. path.join(dir, "html"), "-show-line-counts-or-regions"}, common))
    os.vrunv(llvm_cov, table.join({"export", "-format=lcov"}, common), {stdout = path.join(dir, "lcov.info")})

    local totals = json.decode(os.iorunv(llvm_cov, table.join({"export", "-summary-only"}, common))).data[1].totals
    print_summary({
        {name = "lines", covered = totals.lines.covered, total = totals.lines.count},
        {name = "functions", covered = totals.functions.covered, total = totals.functions.count},
        {name = "regions", covered = totals.regions.covered, total = totals.regions.count},
        {name = "branches", covered = totals.branches.covered, total = totals.branches.count}
    })
end

local function report_gcov(target)
    local gcovr = find_program("gcovr")
    if not gcovr then
        raise("coverage: gcovr not found, install it with `pipx install gcovr` or `pip install gcovr`")
    end

    local dir = outputdir()
    local summary = path.join(dir, "summary.json")
    os.mkdir(path.join(dir, "html"))
    os.vrunv(gcovr, {
        "--root", os.projectdir(),
        "--filter", sourcedir() .. "/",
        "--object-directory", path.absolute(target:objectdir()),
        "--html-details", path.join(dir, "html", "index.html"),
        "--lcov", path.join(dir, "lcov.info"),
        "--json-summary", summary,
        "-j", tostring(os.default_njob()),
        path.absolute(target:objectdir())
    })

    local totals = json.loadfile(summary)
    print_summary({
        {name = "lines", covered = totals.line_covered, total = totals.line_total},
        {name = "functions", covered = totals.function_covered, total = totals.function_total},
        {name = "branches", covered = totals.branch_covered, total = totals.branch_total}
    })
end

function report(target)
    if kind(target) == "llvm" then
        report_llvm(target)
    else
        report_gcov(target)
    end
end
