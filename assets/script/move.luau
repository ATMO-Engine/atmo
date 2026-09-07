right = false
left = false
jump = false
local velocityY = 0

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

    if Input.isJustPressed("Space") then
        jump = true
    end
end

function PhysicsUpdate(deltaTime)
    if jump then
        jump = false;
        this:applyLinearImpulse(Vector2(0, -1000))
    end
end
