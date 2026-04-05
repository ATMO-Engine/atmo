#include "arg_manager.hpp"
#include <utility>
#include "spdlog/spdlog.h"

namespace atmo::core::args
{
    void ArgManager::Parse(int argc, const char *const *argv)
    {
        Instance().m_args.parse_args(argc, argv);
    }

    std::stringstream ArgManager::Help()
    {
        return Instance().m_args.help();
    }

    void ArgManager::AddLaunchHandler(int priority, const std::string &arg, ArgManagerCallback callback)
    {
        Instance().m_launchHandlers[priority].emplace_back(std::make_pair(arg, callback));
    }

    void ArgManager::ExecuteLaunchHandlers()
    {
        ArgManager &instance = Instance();

        for (auto &entry : instance.m_launchHandlers) {
            for (auto &handler : entry.second) {
                if (auto fn = instance.m_args.is_used(handler.first)) {
                    instance.m_launchResult = handler.second(instance);

                    if (instance.m_launchResult != LaunchResult::Continue)
                        return;
                }
            }
        }
    }

    ArgManager::LaunchResult ArgManager::GetLaunchResult()
    {
        return Instance().m_launchResult;
    }


} // namespace atmo::core::args
