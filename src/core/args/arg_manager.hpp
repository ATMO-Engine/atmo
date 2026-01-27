#pragma once

#include <map>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace atmo
{
    namespace core
    {
        namespace args
        {
            using ArgValue = std::variant<std::monostate, bool, int, float, std::string>;
            using ArgPair = struct {
                std::string name;
                ArgValue value;
            };

            class ArgManager
            {
            public:
                static void Parse(int argc, char **argv);
                static void Parse(const std::vector<std::string> &args);
                static bool HasArg(const std::string &name);
                static ArgPair GetArg(const std::string &name);
                static ArgValue GetArgValue(const std::string &name);
                static ArgValue GetArgValueWithDefault(const std::string &name, const ArgValue &default_value = std::monostate());
                static ArgPair GetArgFromIndex(size_t index);
                static ArgValue GetArgValueFromIndex(size_t index);
                static void SetArg(const std::string &name, const ArgValue &value);

                /**
                 * @brief Get all named arguments after the given start argument
                 *
                 * @param start The argument to start from
                 * @return std::vector<std::string>
                 */
                static std::vector<std::string> GetNamedArgs(const std::string &start);

            private:
                ArgManager() = default;
                ~ArgManager() = default;

                static ArgManager &Instance()
                {
                    static ArgManager instance;
                    return instance;
                }

                std::vector<ArgPair> m_args;
            };
        } // namespace args
    } // namespace core
} // namespace atmo
