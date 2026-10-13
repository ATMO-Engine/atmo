#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <random>
#include <thread>

#include <catch2/catch_test_macros.hpp>

#include "file_watcher/file_watcher.hpp"

using namespace std::chrono_literals;
namespace fs = std::filesystem;

namespace
{
    struct TempDir {
        TempDir()
        {
            path = fs::temp_directory_path() / ("atmo_file_watcher_" + std::to_string(std::random_device{}()));
            fs::create_directories(path);
        }

        ~TempDir()
        {
            std::error_code ec;
            fs::remove_all(path, ec);
        }

        fs::path path;
    };

    const atmo::FileWatcher::FileSystemNode *Find(const atmo::FileWatcher::FileSystemNode &root, const fs::path &relative)
    {
        const atmo::FileWatcher::FileSystemNode *current = &root;
        for (const auto &part : relative) {
            auto it = std::ranges::find_if(current->child, [&part](const auto &node) { return node.path.filename() == part; });
            if (it == current->child.end())
                return nullptr;
            current = &(*it);
        }
        return current;
    }

    template <typename Predicate> bool WaitForTree(Predicate predicate, std::chrono::milliseconds timeout = 3000ms)
    {
        auto deadline = std::chrono::steady_clock::now() + timeout;
        while (std::chrono::steady_clock::now() < deadline) {
            atmo::FileWatcher::PollChanges();
            if (predicate(atmo::FileWatcher::GetTree()))
                return true;
            std::this_thread::sleep_for(10ms);
        }
        return false;
    }
} // namespace

TEST_CASE("InitFileSystem scans the existing tree", "[file_watcher]")
{
    TempDir dir;
    fs::create_directories(dir.path / "a" / "b");
    std::ofstream(dir.path / "a" / "b" / "file.txt").close();

    atmo::FileWatcher::InitFileSystem(dir.path);

    const auto &tree = atmo::FileWatcher::GetTree();
    REQUIRE(tree.is_dir);
    REQUIRE(Find(tree, "a/b") != nullptr);
    REQUIRE(Find(tree, "a/b")->is_dir);
    REQUIRE(Find(tree, "a/b/file.txt") != nullptr);
    REQUIRE_FALSE(Find(tree, "a/b/file.txt")->is_dir);
}

TEST_CASE("FileWatcher picks up created files and folders", "[file_watcher]")
{
    TempDir dir;
    atmo::FileWatcher::InitFileSystem(dir.path);

    std::ofstream(dir.path / "new.txt").close();
    REQUIRE(WaitForTree([](const auto &tree) { return Find(tree, "new.txt") != nullptr; }));

    fs::create_directories(dir.path / "x" / "y");
    std::ofstream(dir.path / "x" / "y" / "deep.txt").close();
    REQUIRE(WaitForTree([](const auto &tree) { return Find(tree, "x/y/deep.txt") != nullptr; }));
}

TEST_CASE("FileWatcher picks up deleted and renamed entries", "[file_watcher]")
{
    TempDir dir;
    fs::create_directories(dir.path / "folder");
    std::ofstream(dir.path / "folder" / "old.txt").close();
    std::ofstream(dir.path / "gone.txt").close();
    atmo::FileWatcher::InitFileSystem(dir.path);

    fs::remove(dir.path / "gone.txt");
    REQUIRE(WaitForTree([](const auto &tree) { return Find(tree, "gone.txt") == nullptr; }));

    fs::rename(dir.path / "folder" / "old.txt", dir.path / "folder" / "new.txt");
    REQUIRE(WaitForTree([](const auto &tree) { return Find(tree, "folder/old.txt") == nullptr && Find(tree, "folder/new.txt") != nullptr; }));

    fs::rename(dir.path / "folder", dir.path / "moved");
    REQUIRE(WaitForTree([](const auto &tree) { return Find(tree, "folder") == nullptr && Find(tree, "moved/new.txt") != nullptr; }));

    // Changes inside a moved folder are still reported under its new path.
    std::ofstream(dir.path / "moved" / "after_move.txt").close();
    REQUIRE(WaitForTree([](const auto &tree) { return Find(tree, "moved/after_move.txt") != nullptr; }));
}
