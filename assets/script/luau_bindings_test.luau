function Create()
    print("this =", this)

    local transform = this:getTransform()
    print("transform =", transform)  -- si nil, le composant n'est pas trouvé

    if transform == nil then
        print("transform is nil — entity has no Transform2d")
        return
    end

    print("position =", transform.position)
    print("position.x =", transform.position.x)
end

function Update(deltaTime)
    local transform = this:getTransform()
    if transform == nil then
        return
    end

    transform.position.x = transform.position.x + 100 * deltaTime
end

function PhysiqueUpdate(deltaTime)
end