function Create()
end

function Update(deltaTime)
    local transform = this:getTransform()
    if transform == nil then return end

    if Input.isPressed("D") then
        this:applyLinearVelocity(Vector2(100, 0))
    end

    if Input.isPressed("Q") then
        this:applyLinearVelocity(Vector2(-100, 0))
    end

    if Input.isPressed("Space") then
        this:applyAngularVelocity(45)
    end
end

function PhysicsUpdate(deltaTime)
end
