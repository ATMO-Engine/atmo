#pragma once

#include <string>
#include <utility>

#include "core/resource/pool.hpp"
#include "core/resource/resource.hpp"
#include "core/resource/resource_factory.hpp"

template <typename T> class LoaderRegister
{
public:
    LoaderRegister(const std::string &type)
    {
        atmo::core::resource::ResourceFactory::GetInstance().registerLoader(type, []() { return std::make_shared<T>(); });
    }
};

template <typename Tuple, std::size_t... I> auto createPoolMap(std::index_sequence<I...>)
{
    return std::unordered_map<std::string, atmo::core::resource::Pool>{ { atmo::core::resource::LoaderExtension<std::tuple_element_t<I, Tuple>>::extension,
                                                                          atmo::core::resource::Pool() }... };
}

template <typename Tuple> auto makePoolMap()
{
    return createPoolMap<Tuple>(std::make_index_sequence<std::tuple_size_v<Tuple>>{});
}
