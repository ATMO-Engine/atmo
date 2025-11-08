#pragma once

#include <any>
#include <exception>
#include <string>

namespace atmo
{
    namespace core
    {
        namespace resource
        {
            template<typename T>
            struct LoaderExtension;

            class Resource
            {
            public:
                class LoadException : public std::exception {
                    public:
                        LoadException(const std::string &msg) : m_message("Load exception: " + msg) {};
                        const char *what() const noexcept override{
                            return m_message.c_str();
                        }
                    private:
                        std::string m_message;
                };

                class DestroyException : public std::exception {
                    public:
                        DestroyException(const std::string &msg) : m_message("Destroy exception: " + msg){};
                        const char *what() const noexcept override{
                            return m_message.c_str();
                        }
                    private:
                        std::string m_message;
                };


                virtual ~Resource() = default;

                virtual void load(const std::string &path) = 0;
                virtual void destroy() = 0;

                virtual std::any get() = 0;
            };
        } // namespace resource
    } // namespace core
} // namespace atmo
