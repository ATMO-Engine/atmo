#pragma once

#include <functional>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "argparse/argparse.hpp"
#include "project/project_settings.hpp"

namespace atmo::core::args
{
    class ArgManager
    {
    public:
        /**
         * @brief Result of a launch handler, indicating whether to continue processing other handlers, exit with success, or exit with failure.
         */
        enum class LaunchResult {
            /**
             * @brief Continue processing other handlers. If no other handlers are left, the application will continue launching as normal.
             */
            Continue,

            /**
             * @brief Exit the application immediately with a success status code (0). No further handlers will be processed.
             */
            ExitSuccess,

            /**
             * @brief Exit the application immediately with a failure status code (1). No further handlers will be processed.
             */
            ExitFailure
        };

        /**
         * @brief Predefined patterns for argument counts when adding arguments.
         */
        enum class NargsPattern {
            /**
             * @brief Indicates that the argument is optional and may be provided between 0 and 1 time.
             */
            Optional,

            /**
             * @brief Indicates that the argument may be provided any number of times, including not at all.
             */
            Any,

            /**
             * @brief Indicates that the argument must be provided at least once, but may be provided any number of times.
             */
            AtLeastOne
        };

    private:
        class ArgBuilder
        {
        public:
            template <typename... Args>
            ArgBuilder(const std::string &arg, Args... args) : m_arg(Instance().m_args.add_argument(arg.c_str(), std::forward<Args>(args)...))
            {
            }

            ArgBuilder(argparse::Argument &arg) : m_arg(arg) {}

            ArgBuilder &nargs(std::size_t num_args)
            {
                m_arg.nargs(num_args);

                return *this;
            }

            ArgBuilder &nargs(ArgManager::NargsPattern pattern)
            {
                switch (pattern) {
                    case ArgManager::NargsPattern::Optional:
                        m_arg.nargs(argparse::nargs_pattern::optional);
                        break;
                    case ArgManager::NargsPattern::Any:
                        m_arg.nargs(argparse::nargs_pattern::any);
                        break;
                    case ArgManager::NargsPattern::AtLeastOne:
                        m_arg.nargs(argparse::nargs_pattern::at_least_one);
                        break;
                }

                return *this;
            }

            ArgBuilder &nargs(std::size_t num_args_min, std::size_t num_args_max)
            {
                m_arg.nargs(num_args_min, num_args_max);

                return *this;
            }

            ArgBuilder &help(const std::string &help_text)
            {
                m_arg.help(help_text);

                return *this;
            }

            ArgBuilder &metavar(const std::string &metavar)
            {
                m_arg.metavar(metavar);

                return *this;
            }

            template <typename T> ArgBuilder &defaultValue(const T &value)
            {
                m_arg.default_value(value);

                return *this;
            }

            template <typename T> ArgBuilder &implicitValue(const T &value)
            {
                m_arg.implicit_value(value);

                return *this;
            }

            ArgBuilder &required()
            {
                m_arg.required();

                return *this;
            }

        private:
            argparse::Argument &m_arg;
        };

        class MutuallyExclusiveGroup
        {
        public:
            MutuallyExclusiveGroup(bool required = false) : m_group(Instance().m_args.add_mutually_exclusive_group(required)) {}

            template <typename... Args> ArgBuilder addArgument(const std::string &arg, Args... args)
            {
                ArgBuilder builder = ArgBuilder(m_group.add_argument(arg, std::forward<Args>(args)...));

                return builder;
            }

        private:
            argparse::ArgumentParser::MutuallyExclusiveGroup &m_group;
        };

    public:
        using ArgManagerCallback = std::function<LaunchResult(ArgManager &)>;

        static void Parse(int argc, const char *const *argv);
        static std::stringstream Help();

        static void AddLaunchHandler(int priority, const std::string &arg, ArgManagerCallback callback);
        static void ExecuteLaunchHandlers();

        static LaunchResult GetLaunchResult();

        template <typename... Args> static ArgBuilder AddArgument(const std::string &arg, Args... args)
        {
            return ArgBuilder(arg, std::forward<Args>(args)...);
        }

        template <typename T> static std::optional<T> Present(std::string_view arg_name)
        {
            return Instance().m_args.present<T>(arg_name);
        }

        template <typename T> static T Get(std::string_view arg_name)
        {
            return Instance().m_args.get<T>(arg_name);
        }

        static MutuallyExclusiveGroup AddMutuallyExclusiveGroup(bool required = false)
        {
            return MutuallyExclusiveGroup(required);
        }

        static ArgManager &Instance()
        {
            static ArgManager instance;
            return instance;
        }

    private:
        ArgManager() = default;
        ~ArgManager() = default;

        argparse::ArgumentParser m_args{ "atmo", ATMO_VERSION, argparse::default_arguments::none };
        std::map<int, std::vector<std::pair<std::string, ArgManagerCallback>>, std::greater<int>> m_launchHandlers;
        LaunchResult m_launchResult = LaunchResult::Continue;
    };
} // namespace atmo::core::args
