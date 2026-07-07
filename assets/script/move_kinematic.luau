right = false
left = false
jump = false
local GRAVITY = -784
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
    velocityY = velocityY + GRAVITY * deltaTime
    if velocityY < -784 then
        velocityY = -784
    end

    local vel = this:getLinearVelocity()
    if vel == nil then return end

    local newX = 0
    if right then
        newX = 100
    end
    if left then
        newX = -100
    end
    if jump then
        velocityY = 1000
        jump = false;
    end

    this:applyLinearVelocity(Vector2(newX, velocityY))
end
