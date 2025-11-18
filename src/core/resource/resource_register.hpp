#pragma once

#include <string>
#include <utility>

#include "core/resource/resource_factory.hpp"
#include "core/resource/pool.hpp"
#include "core/resource/resource.hpp"

template<typename T>
class LoaderRegister {
public:
    LoaderRegister(const std::string& type) {
        atmo::core::resource::ResourceFactory::getInstance().registerLoader(type, []() {
            return std::make_shared<T>();
        });
    }
};

//template<typename Tuple, std::size_t... I>
//std::unordered_map<std::string, atmo::core::resource::Pool> createPoolMap(std::index_sequence<I...>) {
//    return std::unordered_map<std::string, atmo::core::resource::Pool>{
//        {atmo::core::resource::LoaderExtension<std::tuple_element_t<I, Tuple>>::extension, atmo::core::resource::Pool()}...
//    };
//}

template<typename Tuple, std::size_t... I>
std::unordered_map<std::string, atmo::core::resource::Pool> createPoolMap(std::index_sequence<I...>) {
        std::unordered_map<std::string, atmo::core::resource::Pool> map;

    (map.emplace(
         std::string(
             atmo::core::resource::LoaderExtension<
                 std::tuple_element_t<I, Tuple>
             >::extension
         ),
         atmo::core::resource::Pool{}
     ), ...);

    return map;
}

template<typename Tuple>
std::unordered_map<std::string, atmo::core::resource::Pool> makePoolMap() {
    return createPoolMap<Tuple>(std::make_index_sequence<std::tuple_size_v<Tuple>>{});
}
