#pragma once

#include <memory>
#include <string>

namespace atmo
{
    namespace core
    {
        namespace resource
        {
            template <typename T> class Resource
            {
            public:
                class LoadException : public std::exception
                {
                public:
                    LoadException(const std::string &msg) : m_message("Load exception: " + msg) {};
                    const char *what() const noexcept override
                    {
                        return m_message.c_str();
                    }

                private:
                    std::string m_message;
                };

                virtual ~Resource() = default;

                /**
                 * @brief
                 * Load the specified resource inside a shared_ptr (the destroyer is a lambda associated to the shared_ptr)
                 *
                 * @param path The path of the resoruce to load
                 * @return std::shared_ptr<T> The resource
                 */
                virtual std::shared_ptr<T> load(const std::string &path) = 0;
            };

            template <typename T, typename Context> class ContextualResource : public Resource<T>
            {
            public:
                std::shared_ptr<T> load(const std::string &) final
                {
                    throw typename Resource<T>::LoadException("requires loadWithContext");
                }

                virtual std::shared_ptr<T> loadWithContext(const std::string &path, Context context) = 0;
            };
        } // namespace resource
    } // namespace core
} // namespace atmo
