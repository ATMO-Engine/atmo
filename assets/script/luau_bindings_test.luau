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
    print("update")
    local transform = this:getTransform()
    if transform == nil then
        print("nil")
        return
    end



    print("before", transform.position.x)
    transform.position.x = transform.position.x + 100 * deltaTime
    print("after", transform.position.x)
end

function PhysiqueUpdate(deltaTime)
end