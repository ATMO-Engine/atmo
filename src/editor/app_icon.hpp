#pragma once

#if !defined(ATMO_EXPORT)

namespace atmo::core::ecs::entities
{
    class Window;
}

namespace atmo::editor
{
    /**
     * @brief Sets the SDL application metadata of the editor, so that desktop environments match its windows with the
     *        `atmo.desktop` entry shipped with the Linux release. Must be called before SDL is initialized.
     */
    void SetAppMetadata();

    /**
     * @brief Gives @p window the editor icon embedded in the binary (`packaging/icons/atmo-256.png`). Does nothing on
     *        macOS, where the icon comes from the application bundle, and in headless mode.
     *
     * @param window Window to give the icon to.
     * @return true if the icon was set.
     */
    bool ApplyAppIcon(core::ecs::entities::Window &window);
} // namespace atmo::editor

#endif
