#include "FileSystem.h"

#include <physfs.h>

#include <algorithm>
#include <limits>
#include <memory>
#include <stdexcept>
#include <utility>

namespace CE::AssetManager {
namespace {
// Code Modification - File System: Keep logical paths portable and prevent aliases and traversal in the save root.
// Scope: path and error helpers. Performance: linear path validation and native checks during configuration only. Toggle: none.
constexpr char ContentRoot[] = "content";
constexpr char SaveRoot[] = "saves";

bool validComponent(std::string_view name)
{
    if (name.empty() || name.size() > 255 || name.back() == '.') {
        return false;
    }
    for (const char character : name) {
        if (!((character >= 'a' && character <= 'z') || (character >= 'A' && character <= 'Z') ||
            (character >= '0' && character <= '9') || character == '.' || character == '-' || character == '_')) {
            return false;
        }
    }
    const auto stem = name.substr(0, name.find('.'));
    if (stem.size() < 3 || stem.size() > 4) {
        return true;
    }
    char prefix[3];
    std::transform(stem.begin(), stem.begin() + 3, prefix, [](char character) {
        return character >= 'a' && character <= 'z' ? static_cast<char>(character - 'a' + 'A') : character;
    });
    const std::string_view code(prefix, 3);
    if (stem.size() == 3) {
        return code != "CON" && code != "PRN" && code != "AUX" && code != "NUL";
    }
    return (code != "COM" && code != "LPT") || stem[3] < '1' || stem[3] > '9';
}

bool validPath(std::string_view path)
{
    if (path.empty() || path.size() > 1024) {
        return false;
    }
    for (;;) {
        const auto slash = path.find('/');
        if (!validComponent(path.substr(0, slash))) {
            return false;
        }
        if (slash == std::string_view::npos) {
            return true;
        }
        path.remove_prefix(slash + 1);
    }
}

FileError physicsError(std::string_view operation)
{
    const auto code = PHYSFS_getLastErrorCode();
    const auto kind = code == PHYSFS_ERR_NOT_FOUND ? FileErrorCode::NotFound : FileErrorCode::Io;
    const char* detail = PHYSFS_getErrorByCode(code);
    return {kind, std::string(operation) + ": " + (detail ? detail : "unknown PhysicsFS error")};
}

FileError invalidPath()
{
    return {FileErrorCode::InvalidPath, "Expected a portable relative logical file path"};
}

FileError savesUnavailable()
{
    return {FileErrorCode::NotConfigured, "Configure a save directory first"};
}

std::string utf8Path(const std::filesystem::path& path)
{
    const auto utf8 = path.u8string();
    return {utf8.begin(), utf8.end()};
}

bool samePath(const std::filesystem::path& first, const std::filesystem::path& second)
{
    std::error_code error;
    return std::filesystem::equivalent(first, second, error);
}

bool containsPath(const std::filesystem::path& root, std::filesystem::path candidate)
{
    for (;;) {
        if (samePath(root, candidate)) {
            return true;
        }
        const auto parent = candidate.parent_path();
        if (parent == candidate || parent.empty()) {
            return false;
        }
        candidate = parent;
    }
}
// End of Code Modification - File System

// Code Modification - File System: Cap whole-file allocations and close every handle along the I/O path.
// Scope: private read implementation. Performance: one allocation proportional to accepted file size. Toggle: caller read limit.
using File = std::unique_ptr<PHYSFS_File, decltype(&PHYSFS_close)>;

FileRead readFile(const std::string& path, std::size_t limit)
{
    File file(PHYSFS_openRead(path.c_str()), &PHYSFS_close);
    if (!file) {
        return physicsError("Open for reading");
    }
    const auto length = PHYSFS_fileLength(file.get());
    if (length < 0) {
        return physicsError("Get file length");
    }
    if (static_cast<PHYSFS_uint64>(length) > std::min(limit, FileBytes{}.max_size())) {
        return FileError{FileErrorCode::TooLarge, "File exceeds the read limit"};
    }
    FileBytes bytes(static_cast<std::size_t>(length));
    if (!bytes.empty()) {
        const auto read = PHYSFS_readBytes(file.get(), bytes.data(), bytes.size());
        if (read < 0) {
            return physicsError("Read file");
        }
        if (read != length) {
            return FileError{FileErrorCode::Io, "Read ended before the reported file length"};
        }
    }
    return bytes;
}
// End of Code Modification - File System
}

// Code Modification - File System: Own the PhysicsFS lifecycle and isolate writable saves from content mounts.
// Scope: owner-configured VFS. Performance: no work until constructed, synchronous I/O per call. Toggle: explicit lifetime and save configuration.
FileSystem::FileSystem()
{
    if (PHYSFS_isInit()) {
        throw std::logic_error("A PhysicsFS session already exists");
    }
    // The supported Windows desktop platform resolves its executable path directly.
    if (!PHYSFS_init(nullptr)) {
        throw std::runtime_error(physicsError("Initialize PhysicsFS").message);
    }
}

FileSystem::~FileSystem()
{
    PHYSFS_deinit();
}

FileStatus FileSystem::mountContent(const std::filesystem::path& source, MountPriority priority)
{
    std::error_code error;
    auto canonical = std::filesystem::canonical(source, error);
    if (error) {
        return FileError{FileErrorCode::InvalidPath, "Resolve content source: " + error.message()};
    }
    if (std::any_of(mounts_.begin(), mounts_.end(), [&](const auto& mount) { return samePath(mount, canonical); })) {
        return FileError{FileErrorCode::Conflict, "Content source is already mounted"};
    }
    if (!saveDirectory_.empty() && (containsPath(canonical, saveDirectory_) || containsPath(saveDirectory_, canonical))) {
        return FileError{FileErrorCode::Conflict, "Content source overlaps the save directory"};
    }
    const auto native = utf8Path(canonical);
    mounts_.push_back(std::move(canonical));
    if (!PHYSFS_mount(native.c_str(), ContentRoot, priority == MountPriority::Last ? 1 : 0)) {
        mounts_.pop_back();
        return physicsError("Mount content");
    }
    return std::nullopt;
}

FileStatus FileSystem::configureSaves(const std::filesystem::path& directory)
{
    if (!saveDirectory_.empty()) {
        return FileError{FileErrorCode::Conflict, "Save directory is already configured"};
    }
    if (!directory.is_absolute()) {
        return FileError{FileErrorCode::InvalidPath, "Expected an absolute save directory"};
    }
    std::error_code error;
    // Check the prospective location before creating any directories inside a mounted source.
    auto canonical = std::filesystem::weakly_canonical(directory, error);
    if (error) {
        return FileError{FileErrorCode::InvalidPath, "Resolve save directory: " + error.message()};
    }
    for (const auto& mount : mounts_) {
        if (containsPath(mount, canonical) || containsPath(canonical, mount)) {
            return FileError{FileErrorCode::Conflict, "Save directory overlaps a content source"};
        }
    }
    std::filesystem::create_directories(canonical, error);
    if (error) {
        return FileError{FileErrorCode::Io, "Create save directory: " + error.message()};
    }
    const auto native = utf8Path(canonical);
    if (!PHYSFS_mount(native.c_str(), SaveRoot, 1)) {
        return physicsError("Mount save directory");
    }
    if (!PHYSFS_setWriteDir(native.c_str())) {
        const auto code = PHYSFS_getLastErrorCode();
        PHYSFS_unmount(native.c_str());
        const char* detail = PHYSFS_getErrorByCode(code);
        return FileError{FileErrorCode::Io, std::string("Set save write directory: ") + (detail ? detail : "unknown PhysicsFS error")};
    }
    saveDirectory_ = std::move(canonical);
    return std::nullopt;
}

FileStatus FileSystem::configureUserSaves()
{
    if (!saveDirectory_.empty()) {
        return FileError{FileErrorCode::Conflict, "Save directory is already configured"};
    }
    const auto* directory = PHYSFS_getPrefDir("WiseEngine", "WiseEngine");
    if (!directory) {
        return physicsError("Resolve user save directory");
    }
    const std::string_view utf8(directory);
    return configureSaves(std::filesystem::path(std::u8string(utf8.begin(), utf8.end())));
}

FileRead FileSystem::readContent(std::string_view path, std::size_t limit) const
{
    if (!validPath(path)) {
        return invalidPath();
    }
    return readFile(std::string(ContentRoot) + '/' + std::string(path), limit);
}

FileRead FileSystem::readSave(std::string_view path, std::size_t limit) const
{
    if (!validPath(path)) {
        return invalidPath();
    }
    if (saveDirectory_.empty()) {
        return savesUnavailable();
    }
    return readFile(std::string(SaveRoot) + '/' + std::string(path), limit);
}

FileStatus FileSystem::writeSave(std::string_view path, std::span<const std::byte> bytes) const
{
    if (!validPath(path)) {
        return invalidPath();
    }
    if (saveDirectory_.empty()) {
        return savesUnavailable();
    }
    if (bytes.size() > static_cast<PHYSFS_uint64>(std::numeric_limits<PHYSFS_sint64>::max())) {
        return FileError{FileErrorCode::TooLarge, "Save exceeds the PhysicsFS write limit"};
    }
    const std::string logical(path);
    if (const auto slash = logical.rfind('/'); slash != std::string::npos &&
        !PHYSFS_mkdir(logical.substr(0, slash).c_str())) {
        return physicsError("Create save subdirectory");
    }
    File file(PHYSFS_openWrite(logical.c_str()), &PHYSFS_close);
    if (!file) {
        return physicsError("Open save for writing");
    }
    if (!bytes.empty()) {
        const auto written = PHYSFS_writeBytes(file.get(), bytes.data(), bytes.size());
        if (written < 0) {
            return physicsError("Write save");
        }
        if (static_cast<PHYSFS_uint64>(written) != bytes.size()) {
            return FileError{FileErrorCode::Io, "Save write was incomplete"};
        }
    }
    if (!PHYSFS_close(file.get())) {
        return physicsError("Close save after writing");
    }
    (void)file.release();
    return std::nullopt;
}
// End of Code Modification - File System
}
