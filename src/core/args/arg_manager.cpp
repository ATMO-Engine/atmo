#include "arg_manager.hpp"
#include "spdlog/spdlog.h"

namespace atmo
{
    namespace core
    {
        namespace args
        {
            void ArgManager::Parse(int argc, char **argv)
            {
                std::vector<std::string> args;
                for (int i = 1; i < argc; ++i) {
                    args.emplace_back(argv[i]);
                }
                Parse(args);
            }

            void ArgManager::Parse(std::vector<std::string> args)
            {
                Instance().m_args.clear();

                for (size_t i = 0; i < args.size(); ++i) {
                    const std::string &arg = args[i];
                    if (arg.starts_with("--")) {
                        std::string name, value_str;
                        ArgValue value = true;
                        size_t equal_pos = arg.find('=');
                        if (equal_pos != std::string::npos) {
                            name = arg.substr(2, equal_pos - 2);
                            value_str = arg.substr(equal_pos + 1);
                        } else {
                            name = arg.substr(2);
                            if (i + 1 < args.size() && !args[i + 1].starts_with("-") && !args[i + 1].starts_with("--")) {
                                value_str = args[i + 1];
                                ++i;
                            }
                        }

                        if (!value_str.empty()) {
                            try {
                                std::size_t pos;
                                value = std::stoi(value_str, &pos);
                                if (pos != value_str.size()) {
                                    throw std::invalid_argument("Not fully parsed as int");
                                }
                            } catch (...) {
                                try {
                                    value = std::stof(value_str);
                                } catch (...) {
                                    value = value_str;
                                }
                            }
                        }

                        Instance().m_args.push_back({ name, value });
                    } else if (arg.starts_with("-")) {
                        for (size_t j = 1; j < arg.size(); ++j) {
                            std::string name(1, arg[j]);
                            Instance().m_args.push_back({ name, true });
                        }
                    } else {
                        Instance().m_args.push_back({ arg, std::monostate() });
                    }
                }
            }

            bool ArgManager::HasArg(const std::string &name)
            {
                for (const auto &arg : Instance().m_args) {
                    if (arg.name == name) {
                        return true;
                    }
                }
                return false;
            }

            ArgPair ArgManager::GetArg(const std::string &name)
            {
                for (const auto &arg : Instance().m_args) {
                    if (arg.name == name) {
                        return arg;
                    }
                }
                throw std::runtime_error("Argument not found: " + name);
            }

            ArgValue ArgManager::GetArgValue(const std::string &name)
            {
                for (const auto &arg : Instance().m_args) {
                    if (arg.name == name) {
                        return arg.value;
                    }
                }
                throw std::runtime_error("Argument not found: " + name);
            }

            ArgValue ArgManager::GetArgValueWithDefault(const std::string &name, const ArgValue &default_value)
            {
                for (const auto &arg : Instance().m_args) {
                    if (arg.name == name) {
                        return arg.value;
                    }
                }
                return default_value;
            }

            ArgPair ArgManager::GetArgFromIndex(size_t index)
            {
                if (index < Instance().m_args.size()) {
                    return Instance().m_args[index];
                }
                throw std::runtime_error("Argument index out of range: " + std::to_string(index));
            }

            ArgValue ArgManager::GetArgValueFromIndex(size_t index)
            {
                if (index < Instance().m_args.size()) {
                    return Instance().m_args[index].value;
                }
                throw std::runtime_error("Argument index out of range: " + std::to_string(index));
            }

            void ArgManager::SetArg(const std::string &name, const ArgValue &value)
            {
                for (auto &arg : Instance().m_args) {
                    if (arg.name == name) {
                        arg.value = value;
                        return;
                    }
                }
                Instance().m_args.push_back({ name, value });
            }

        } // namespace args
    } // namespace core
} // namespace atmo
