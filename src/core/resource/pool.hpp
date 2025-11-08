#pragma once

#include <cstdint>
#include <exception>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "core/resource/resource.hpp"
#include "core/resource/handle.hpp"

namespace atmo
{
    namespace core
    {
        namespace resource
        {
            class Pool
            {
            public:
                class HandleOutDated : public std::exception {
                    public:
                        HandleOutDated(const std::string &msg) : m_message(msg) {};
                        const char *what() const noexcept override {
                            return m_message.c_str();
                        }
                    private:
                        std::string m_message;
                };


                Pool();
                ~Pool();

                const Handle create(const std::string &path);
                std::shared_ptr<Resource> getFromHandle(const Handle &handle);

                void declareHandle(const Handle &handle);
                void destroy(const Handle &handle);
            private:
                std::vector<Handle> _usedHandles;
                std::unordered_map<std::string, Handle> _handles;

                std::vector<std::shared_ptr<Resource>> _resources;
                std::vector<std::uint16_t> _generations;
                std::vector<std::uint16_t> _freeList;
            };
        } // namespace resource
    } // namespace core
} // namespace atmo
