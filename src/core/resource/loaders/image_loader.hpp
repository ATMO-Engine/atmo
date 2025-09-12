#pragma once

#include <memory>
#include "core/resource/resource_loader.hpp"

namespace atmo
{
    namespace core
    {
        namespace resource
        {
            class ImageLoader : public ResourceLoader
            {
            public:           
                ImageLoader(); 
              //  std::shared_ptr<void> &get(const std::string &path);
            };
        } // namespace resource
    } // namespace core
} // namespace atmo
