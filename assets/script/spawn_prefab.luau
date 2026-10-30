-- Spawns a prefab every SPAWN_INTERVAL seconds, next to this entity.
--
-- Setup: attach this script to an entity that has a Transform2d, and set PREFAB_PATH
-- to a .prefab file saved from the scene editor.

local PREFAB_PATH = "project://assets/prefabs/enemy.prefab"
local SPAWN_INTERVAL = 10
local SPAWN_RADIUS = 200

local timer = 0
local spawned = 0

local function spawn()
    local parent = this

    Prefab.instantiate(PREFAB_PATH, parent, function(entity)
        spawned += 1

        local spawnerTransform = this:getTransform()
        local transform = entity:getTransform()
        if spawnerTransform == nil or transform == nil then return end

        -- Read the spawner position now: it is its position at the end of this frame
        local newPos = Vector2(
            spawnerTransform.position.x + math.random(-SPAWN_RADIUS, SPAWN_RADIUS),
            spawnerTransform.position.y - 200
        )
        transform.position = newPos

        print("Spawned " .. entity:name() .. " (" .. spawned .. ")")
    end)
end

function Create()
    spawn()
end

function Update(deltaTime)
    timer += deltaTime
    if timer >= SPAWN_INTERVAL then
        timer -= SPAWN_INTERVAL
        spawn()
    end
end
