right = false
left = false
jump = false

function Create()
end

function Update(deltaTime)
    local transform = this:getTransform()
    if transform == nil then return end

    if Input.isPressed("D") then
        right = true
    else
        right = false
    end

    if Input.isPressed("Q") then
        left = true
    else
        left = false
    end

    if Input.isPressed("Space") then
        jump = true
    else
        jump = false;
    end
end

function PhysicsUpdate(deltaTime)
    if right then
        this:applyLinearVelocity(Vector2(100, 0))
    end
    if left then
        this:applyLinearVelocity(Vector2(-100, 0))
    end
    if jump then
        this:applyAngularVelocity(45)
    end
end
