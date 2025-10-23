#pragma once

#include <filesystem>
#include <fstream>

#include "file_system.hpp"
#include "project/project_settings.hpp"

#define ATMO_PROJECT_FILE "project.atmo"
#define ATMO_PACKED_EXT ".pck"

class ProjectManager
{
public:
    /**
     * @brief Opens a project from the path to a project.atmo file.
     *
     * Initializes the project environment by loading the project settings,
     * resources, and scenes defined in the project.atmo file.
     *
     * @param path The filesystem path to the project.atmo file.
     */
    inline static void OpenProject(const std::filesystem::path &path)
    {
#if defined(ATMO_EXPORT)
        throw std::runtime_error("Cannot open project of an exported application.");
#endif
        if (!std::filesystem::exists(path) || path.filename() != ATMO_PROJECT_FILE)
            throw std::runtime_error("Invalid project file path: " + path.string());

        FileSystem::SetRootPath(path);
    }

    /**
     * @brief Creates a new project at the specified directory path.
     *
     * Creates a default project.atmo file into the specified directory,
     * setting up the necessary structure and default settings for a new project.
     *
     * @param path The filesystem path where the new project.atmo file will be created. Has to be a directory.
     *
     * @return The path to the created project.atmo file.
     */
    static std::filesystem::path CreateProject(const std::filesystem::path &path);

    /**
     * @brief Get the Current Project Path object
     *
     * @return The filesystem path of the currently opened project.
     */
    inline static std::filesystem::path GetCurrentProjectPath()
    {
        return FileSystem::GetRootPath();
    }

    /**
     * @brief Closes the currently opened project.
     *
     */
    inline static void CloseProject()
    {
#if defined(ATMO_EXPORT)
        throw std::runtime_error("Cannot close project of an exported application.");
#endif
    }

    /**
     * @brief Generates a packed .pck file from the current project directory.
     *
     */
    static void GeneratePackedFile()
    {
#if !defined(ATMO_EXPORT)
        throw std::runtime_error("Cannot generate packed file from an exported application.");
#endif
        std::ofstream out(GetCurrentProjectPath() / m_instance.m_settings.app.project_name / ATMO_PACKED_EXT, std::ios::binary);
        if (!out.is_open())
            throw std::runtime_error("Failed to create packed file.");

        out.write(reinterpret_cast<const char *>(&m_instance.m_settings), sizeof(m_instance.m_settings));

        // TODO: Write resources and scenes to the packed file

        out.close();
    };

private:
    ProjectManager() = default;
    ~ProjectManager() = default;

    static ProjectManager m_instance;

    ProjectSettings m_settings;

    // inline static m_LoadProjectSettings
};
