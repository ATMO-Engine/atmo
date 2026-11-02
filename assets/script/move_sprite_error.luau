-- Triggers the "value is a copy" error: transform.position returns a copy of the Vector2,
-- so modifying its fields is refused instead of silently doing nothing.
-- Expected log when pressing D, Q, S or Z:
--   'x' can't be modified, this value is a copy: create a new one instead (e.g. transform.position = Vector2(x, y))
-- See move_sprite.luau for the correct way.

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

    if Input.isPressed("S") then
        transform.position.y = transform.position.y + 100 * deltaTime
    end

    if Input.isPressed("Z") then
        transform.position.y = transform.position.y - 100 * deltaTime
    end
end

function PhysicsUpdate(deltaTime)
end
