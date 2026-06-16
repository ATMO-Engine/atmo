function Create()
end

function Update(deltaTime)
    local transform = this:getTransform()
    if transform == nil then return end

    if Input.isPressed("D") then
        transform.position.x = transform.position.x + 100 * deltaTime
    end

    if Input.isPressed("Q") then
        transform.position.x = transform.position.x - 100 * deltaTime
    end
end

function PhysicsUpdate(deltaTime)
end

