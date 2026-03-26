local x = 0

local updateTimer = 0
local physicsTimer = 0

function Create()
    x = 10
end

function Update(deltaTime)
    updateTimer += deltaTime

    if updateTimer >= 2 then
        updateTimer -= 2 -- keep remainder for accuracy
        x = x + 1
    end
end

function PhysiqueUpdate(deltaTime)
    physicsTimer += deltaTime

    if physicsTimer >= 1 then
        physicsTimer -= 1 -- keep remainder
        print(x)
    end
end
