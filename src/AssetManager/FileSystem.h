#pragma once

#include <cstddef>
#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace CE::AssetManager {
// Code Modification - File System: Separate logical asset names from trusted native configuration paths.
// Scope: one process-wide VFS session. Performance: synchronous I/O and bounded whole-file reads.
// Toggle: construct only when filesystem access is needed.
enum class FileErrorCode { InvalidPath, NotFound, TooLarge, NotConfigured, Conflict, Io };

struct FileError {
    FileErrorCode code;
    std::string message;
};

enum class MountPriority { First, Last };
using FileStatus = std::optional<FileError>;
using FileBytes = std::vector<std::byte>;
using FileRead = std::variant<FileBytes, FileError>;

class FileSystem {
public:
    static constexpr std::size_t DefaultReadLimit = 64 * 1024 * 1024;

    FileSystem();
    ~FileSystem();
    FileSystem(const FileSystem&) = delete;
    FileSystem& operator=(const FileSystem&) = delete;

    FileStatus mountContent(const std::filesystem::path& source, MountPriority priority = MountPriority::Last);
    FileStatus configureSaves(const std::filesystem::path& directory);
    FileStatus configureUserSaves();

    FileRead readContent(std::string_view path, std::size_t limit = DefaultReadLimit) const;
    FileRead readSave(std::string_view path, std::size_t limit = DefaultReadLimit) const;
    FileStatus writeSave(std::string_view path, std::span<const std::byte> bytes) const;

private:
    std::vector<std::filesystem::path> mounts_;
    std::filesystem::path saveDirectory_;
};
// End of Code Modification - File System
}
