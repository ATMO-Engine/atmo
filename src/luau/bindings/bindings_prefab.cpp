#include "bindings_prefab.hpp"
#include <memory>
#include <string>
#include "core/ecs/entities/entity.hpp"
#include "core/ecs/prefab.hpp"
#include "luau/bindings/EntityLuauRegistry.hpp"
#include "luau/bindings/entity/bindings_entity.hpp"
#include "luau/luau.hpp"
#include "luau/luau_ref.hpp"

namespace atmo::luau
{
    int PrefabBindings::Instantiate(lua_State *state)
    {
        /** stack should be: [path, parent, callback?] */
        std::string path = luaL_checkstring(state, 1);
        auto *parent = LuaBindings<core::ecs::entities::Entity>::CheckPtr(state, 2);

        auto callback = std::make_shared<LuauRef>(&Luau::Instance());
        if (!lua_isnoneornil(state, 3)) {
            luaL_checktype(state, 3, LUA_TFUNCTION);
            callback->set(lua_ref(state, 3));
        }

        core::SignalQueue::Enqueue([path = std::move(path), parent = *parent, callback]() {
            auto root = core::ecs::Prefab::Instantiate(path, parent);
            if (!root || callback->getRef() == LUA_NOREF)
                return;

            lua_State *L = Luau::Instance().getState();
            lua_getref(L, callback->getRef());

            flecs::entity handle = root->getHandle();
            if (!LuauRegistry::Instance().pushEntity(L, handle)) {     // pushes the root as the userdata of its most specific bound type     | stack: [..., cb, root]
                lua_pop(L, 1);                                         // no binding found, nothing was pushed: drop the callback             | stack: [...]
                return;
            }

            Luau::ProtectedCall(L, 1, "Prefab.instantiate callback");
        });

        return 0;
    }
} // namespace atmo::luau
