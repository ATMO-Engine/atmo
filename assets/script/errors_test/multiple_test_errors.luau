local frame = 0

function Create()
end

function Update(deltaTime)
    frame = frame + 1

    if frame == 1 then
        local transform = nil
        local x = transform.position.x -- attempt to index nil value 'transform'

    elseif frame == 2 then
        Input.isPressed(42) -- mauvais type d'argument

    elseif frame == 3 then
        local bad = "hello" + 5 -- attempt to perform arithmetic

    elseif frame == 4 then
        this:nonExistentMethod() -- attempt to call a nil value (method 'nonExistentMethod')

    elseif frame == 5 then
        NonExistentGlobalFunction() -- attempt to call a nil value (global)

    elseif frame == 6 then
        local t = Transform2d()
        t.position = "not_a_vector2" -- CheckPtr échoue -> ton code fait juste "return" silencieusement, pas d'erreur visible !

    elseif frame == 7 then
        local v = Vector2(1, "deux") -- luaL_checknumber échoue sur "deux"
    end
end

function PhysicsUpdate(deltaTime)
end