#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "core/resource/loaders/Ipool.hpp"
#include "core/resource/handle.hpp"

namespace atmo
{
    namespace core
    {
        namespace resource
        {

            struct Bytecode
            {
                char *data;
                size_t size;
            };

            class ScriptPool : public IPool
            {
            public:
                ScriptPool();
                ~ScriptPool();

                const Handle create(const std::string &path);
                std::any getFromHandle(const Handle &handle);

                void destroy(const Handle &handle);
            private:
                std::vector<Bytecode> _resources;
                std::vector<std::uint16_t> _generations;
                std::vector<std::uint16_t> _freeList;
            };
        } // namespace resource
    } // namespace core
} // namespace atmo
