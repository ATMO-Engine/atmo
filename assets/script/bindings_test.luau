-- Luau bindings test
--
-- Setup: attach this script to an entity that has a Transform2d (Entity2d, Sprite2d, Body2d...)
-- and give it a child entity named "Target" that also has a Transform2d.
--
-- 1. Automatic: the checks below run in Create, each one logs [PASS] or [FAIL], then a summary.
-- 2. Manual: while the game runs, delete "Target" in the editor. The script keeps Target's
--    transform in a variable, the next access must log [PASS] with a "destroyed" error instead of crashing.

local passed, failed = 0, 0

local function check(name, ok, detail)
    if ok then
        passed += 1
        print("[PASS] " .. name)
    else
        failed += 1
        print("[FAIL] " .. name .. (if detail ~= nil then ": " .. tostring(detail) else ""))
    end
end

-- fn must raise an error, whose message contains `expected` when given
local function expectError(name, fn, expected)
    local ok, err = pcall(fn)
    if ok then
        check(name, false, "no error raised")
        return
    end
    check(name, expected == nil or string.find(tostring(err), expected, 1, true) ~= nil, err)
end

local function expectOk(name, fn)
    local ok, err = pcall(fn)
    check(name, ok, err)
end

local function near(a, b)
    return math.abs(a - b) < 1e-4
end

local targetTransform = nil
local targetDone = false
local frame = 0

function Create()
    print("=== Bindings test ===")

    -- Metatables are locked
    check("getmetatable(this) is locked", getmetatable(this) == "The metatable is locked", getmetatable(this))
    check("getmetatable(Vector2) is locked", getmetatable(Vector2(0, 0)) == "The metatable is locked")

    -- Vector2 / Color are read-only values
    local v = Vector2(3, 4)
    check("Vector2 read", v.x == 3 and v.y == 4, tostring(v.x) .. ", " .. tostring(v.y))
    check("Vector2:length()", near(v:length(), 5), v:length())
    expectError("Vector2.x is read-only", function() v.x = 1 end, "can't be modified")
    expectError("Vector2 unknown field write", function() v.z = 1 end, "is not a property")
    check("Vector2 unknown field read is nil", v.z == nil)

    local c = Color(0.1, 0.2, 0.3, 1)
    check("Color read", near(c.g, 0.2), c.g)
    expectError("Color.r is read-only", function() c.r = 1 end, "can't be modified")

    -- Transform of this entity
    local t = this:getTransform()
    check("this:getTransform() not nil", t ~= nil)
    if t ~= nil then
        expectError("transform.position.x = ... fails loudly", function() t.position.x = 1 end, "can't be modified")
        expectError("transform unknown field write", function() t.positon = Vector2(0, 0) end, "is not a property")

        local before = t.position
        t.position = Vector2(before.x + 10, before.y)
        check("position write goes to the component", near(t.position.x, before.x + 10), t.position.x)
        check("a read position is a copy", near(before.x, t.position.x - 10), before.x)
        t.position = before

        local r = t.rotation
        t.rotation = r + 1
        check("rotation write", near(t.rotation, r + 1), t.rotation)
        t.rotation = r
    end

    -- Transform2d created by the script (owned by Lua)
    local owned = Transform2d()
    owned.position = Vector2(5, 6)
    check("owned Transform2d write/read", owned.position.x == 5 and owned.position.y == 6)

    -- Entity checks
    check("this:isAlive()", this:isAlive() == true)
    expectError("entity method with a wrong type errors", function() this.getTransform(Vector2(0, 0)) end, "Entity expected")
    expectError("entity method with nil errors", function() this.getTransform(nil) end, "Entity expected")

    -- Lua memory is reclaimed for values (only the Lua side can be measured from here)
    collectgarbage("collect")
    local memBefore = collectgarbage("count")
    for i = 1, 100000 do
        local _ = Vector2(i, i)
    end
    collectgarbage("collect")
    local grownKb = collectgarbage("count") - memBefore
    check("100k Vector2 are collected", grownKb < 100, string.format("%.1f KB kept", grownKb))

    -- Child used by the manual test
    local target = this:getChild("Target")
    if target == nil then
        print("[SKIP] no child named 'Target', the destroyed entity test won't run")
    else
        targetTransform = target:getTransform()
        check("child Target has a transform", targetTransform ~= nil)
        print("Manual step: delete 'Target' in the editor now")
    end

    print(string.format("=== %d passed, %d failed ===", passed, failed))
end

function Update(deltaTime)
    if targetTransform == nil or targetDone then
        return
    end

    frame += 1
    if frame % 60 ~= 0 then
        return
    end

    -- Once Target is deleted, this must be a Lua error, not a crash
    local ok, err = pcall(function()
        return targetTransform.position
    end)
    if not ok then
        targetDone = true
        check("destroyed Target transform errors cleanly", string.find(tostring(err), "destroyed", 1, true) ~= nil, err)
    end
end
