#include "focus_manager.hpp"
#include <utility>
#include "core/input/input_manager.hpp"

namespace atmo::core
{
    FocusManager &FocusManager::Instance()
    {
        static FocusManager instance;
        return instance;
    }

    void FocusManager::Focus(Target target)
    {
        auto &self = Instance();
        if (self.m_target.id == target.id && target.id != 0) {
            self.m_target = std::move(target);
            return;
        }

        Blur();
        self.m_target = std::move(target);
        if (self.m_target.window)
            InputManager::StartTextInput(self.m_target.window);
    }

    void FocusManager::Blur()
    {
        auto &self = Instance();
        if (self.m_target.id == 0)
            return;

        auto previous = std::exchange(self.m_target, {});
        if (previous.window)
            InputManager::StopTextInput(previous.window);
        if (previous.on_blur)
            previous.on_blur();
    }

    void FocusManager::Release(std::uint64_t id) noexcept
    {
        auto &self = Instance();
        if (self.m_target.id != id || id == 0)
            return;
        if (self.m_target.window)
            InputManager::StopTextInput(self.m_target.window);
        self.m_target = {};
    }

    bool FocusManager::IsFocused(std::uint64_t id) noexcept
    {
        return id != 0 && Instance().m_target.id == id;
    }

    bool FocusManager::HasTextFocus() noexcept
    {
        return Instance().m_target.id != 0;
    }

    std::uint64_t FocusManager::GetFocusedId() noexcept
    {
        return Instance().m_target.id;
    }

    bool FocusManager::DispatchEditAction(text::EditAction action)
    {
        auto &self = Instance();
        if (self.m_target.id == 0 || !self.m_target.on_action)
            return false;
        return self.m_target.on_action(action);
    }
} // namespace atmo::core
