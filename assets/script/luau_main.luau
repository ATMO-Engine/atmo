local updateTimer = 0   -- If this is not present the program crash what should the behavior be ?
local physicsTimer = 0  -- If this is not present the program crash what should the behavior be ?

local v = nil

function Create()
    print("=== CREATE ===")

    -- Création du vecteur
    v = Vector2(3, 4)
    print("Vector created")

    -- Lecture des valeurs
    print("v.x =", v.x)
    print("v.y =", v.y)

    -- Test méthode length
    print("v:length() =", v:length())

    -- Modification
    v.x = 10
    v.y = 20

    print("After modification:")
    print("v.x =", v.x)
    print("v.y =", v.y)

    print("v:length() =", v:length())
end

function Update(deltaTime)
    updateTimer += deltaTime

    if updateTimer >= 2 then
        updateTimer -= 2

        print("=== UPDATE ===")
        print("x =", x)

        -- Modifier le vecteur dynamiquement
        v.x = v.x + 1
        v.y = v.y + 2

        print("Vector updated:")
        print("v.x =", v.x)
        print("v.y =", v.y)
    end
end

function PhysiqueUpdate(deltaTime)
    physicsTimer += deltaTime

    if physicsTimer >= 1 then
        physicsTimer -= 1

        print("=== PHYSICS ===")
        print("x =", x)

        if v then
            print("Vector state:")
            print("v.x =", v.x)
            print("v.y =", v.y)
            print("v:length() =", v:length())
        end
    end
end
