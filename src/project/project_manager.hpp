#pragma once

#include <filesystem>
#include <fstream>
#include <semver.hpp>
#include <spdlog/spdlog.h>

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
#else
        if (!std::filesystem::exists(path) || path.filename() != ATMO_PROJECT_FILE)
            throw std::runtime_error("Invalid project file path: " + path.string());

        FileSystem::SetRootPath(path);
        File project_file = FileSystem::OpenFile(path.string());
        LoadProjectSettings(project_file);

        if (std::memcmp(m_instance.m_settings.atmo_signature, "ATMO", 4) != 0) {
            throw std::runtime_error("Invalid project file: incorrect signature.");
        }

        semver::version<VERSION_TYPES> current_engine_version;
        semver::parse(ATMO_VERSION_STRING, current_engine_version);

        if (m_instance.m_settings.app.engine_version < current_engine_version) {
            spdlog::warn(
                "Project engine version ({}) is older than the current engine version ({}). It will be overwritten on save.",
                m_instance.m_settings.app.engine_version.to_string(),
                current_engine_version.to_string());
        }
#endif
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
    static std::filesystem::path CreateProject(const std::filesystem::path &path)
    {
#if defined(ATMO_EXPORT)
        throw std::runtime_error("Cannot create project of an exported application.");
#else
        if (!std::filesystem::exists(path))
            std::filesystem::create_directories(path);

        std::filesystem::path project_file_path = path / ATMO_PROJECT_FILE;
        if (std::filesystem::exists(project_file_path))
            throw std::runtime_error("Project file already exists at: " + project_file_path.string());

        m_instance.m_settings = ProjectSettings();
        semver::parse(ATMO_VERSION_STRING, m_instance.m_settings.app.engine_version);

        std::ofstream project_file(project_file_path, std::ios::binary);
        if (!project_file.is_open())
            throw std::runtime_error("Failed to create project file at: " + project_file_path.string());

        WriteProjectSettings(project_file);
        project_file.close();

        return project_file_path;
#endif
    }

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
#if defined(ATMO_EXPORT)
        throw std::runtime_error("Cannot generate packed file from an exported application.");
#else
        std::ofstream out(GetCurrentProjectPath() / (std::string(m_instance.m_settings.app.project_name) + std::string(ATMO_PACKED_EXT)), std::ios::binary);
        if (!out.is_open())
            throw std::runtime_error("Failed to create packed file.");

        WriteProjectSettings(out);

        // TODO: Write resources and scenes to the packed file

        out.close();
#endif
    };

private:
    ProjectManager() = default;
    ~ProjectManager() = default;

    static ProjectManager m_instance;

    ProjectSettings m_settings;

    inline static void LoadProjectSettings(File &file)
    {
        file.read(reinterpret_cast<char *>(&m_instance.m_settings), sizeof(ProjectSettings));
    }

    inline static void WriteProjectSettings(std::ofstream &file)
    {
        file.write(reinterpret_cast<const char *>(&m_instance.m_settings), sizeof(ProjectSettings));
    }
};
