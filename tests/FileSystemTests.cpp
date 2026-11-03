#include "AssetManager/FileSystem.h"

#include <array>
#include <iostream>
#include <stdexcept>
#include <utility>

namespace {
// Code Modification - Tests: Exercise real loose files, ZIP mounts and isolated save writes.
// Scope: VFS integration test. Performance: test-only disk I/O. Toggle: WISEENGINE_BUILD_TESTS.
using namespace CE::AssetManager;

void require(bool condition, const char* message)
{
    if (!condition) {
        throw std::runtime_error(message);
    }
}

void requireOk(const FileStatus& status)
{
    if (status) {
        throw std::runtime_error(status->message);
    }
}

void requireCode(const FileRead& result, FileErrorCode code)
{
    const auto* error = std::get_if<FileError>(&result);
    if (!error || error->code != code) {
        throw std::runtime_error(error ? error->message : "Expected a file error");
    }
}

void requireBytes(const FileRead& result, std::string_view expected)
{
    const auto* bytes = std::get_if<FileBytes>(&result);
    if (!bytes) {
        throw std::runtime_error(std::get<FileError>(result).message);
    }
    require(std::string_view(expected).size() == bytes->size(), "File byte count differs");
    for (std::size_t index = 0; index < bytes->size(); ++index) {
        require((*bytes)[index] == static_cast<std::byte>(expected[index]), "File byte content differs");
    }
}

void verify(const std::filesystem::path& root)
{
    const auto saves = root / u8"saves-\u00e9";
    constexpr auto sample = "terrain/height.bin";
    {
        FileSystem files;
        requireCode(files.readSave(sample), FileErrorCode::NotConfigured);
        const auto unconfigured = files.writeSave(sample, {});
        require(unconfigured && unconfigured->code == FileErrorCode::NotConfigured, "Write required save configuration");
        const auto missing = files.mountContent(root / "missing");
        require(missing && missing->code == FileErrorCode::InvalidPath, "Missing native source was accepted");
        const auto bad = files.mountContent(root / "broken.zip");
        require(bad && bad->code == FileErrorCode::Io, "Invalid archive was accepted");

        requireOk(files.mountContent(root / "loose"));
        const auto duplicate = files.mountContent(root / "loose" / ".");
        require(duplicate && duplicate->code == FileErrorCode::Conflict, "Duplicate source was accepted");
        const auto nested = files.configureSaves(root / "loose" / "nested-saves");
        require(nested && nested->code == FileErrorCode::Conflict &&
            !std::filesystem::exists(root / "loose" / "nested-saves"), "Save root must not overlap content");
        requireBytes(files.readContent(sample), "height");
        requireCode(files.readContent(sample, 5), FileErrorCode::TooLarge);
        requireBytes(files.readContent("empty.bin", 0), "");
        requireCode(files.readContent("missing.bin"), FileErrorCode::NotFound);

        for (const auto path : {"", "/outside", "a//b", "a/../b", "a/./b", "a/", "C:/file", "a\\b", "nul.txt", "a."}) {
            requireCode(files.readContent(path), FileErrorCode::InvalidPath);
            const auto error = files.writeSave(path, {});
            require(error && error->code == FileErrorCode::InvalidPath, "Unsafe save name was accepted");
        }
        requireCode(files.readContent(std::string_view("a\0b", 3)), FileErrorCode::InvalidPath);

        requireOk(files.mountContent(root / "content.zip", MountPriority::First));
        requireBytes(files.readContent(sample), "height");
        requireBytes(files.readContent("priority.txt"), "zip");
        requireBytes(files.readContent("loose-only.txt"), "loose");
        requireOk(files.configureSaves(saves));
        const auto overlaps = files.mountContent(root);
        require(overlaps && overlaps->code == FileErrorCode::Conflict, "Parent content mount would expose saves");
        requireCode(files.readSave("priority.txt"), FileErrorCode::NotFound);

        const std::array initial{std::byte{'a'}, std::byte{'b'}};
        requireOk(files.writeSave("profiles/player.bin", initial));
        requireBytes(files.readSave("profiles/player.bin"), "ab");
        requireCode(files.readContent("profiles/player.bin"), FileErrorCode::NotFound);
        requireOk(files.writeSave("profiles/player.bin", std::span(initial).first(1)));
        requireBytes(files.readSave("profiles/player.bin"), "a");
        requireOk(files.writeSave("profiles/empty.bin", {}));
        requireBytes(files.readSave("profiles/empty.bin", 0), "");
    }
    {
        FileSystem files;
        requireOk(files.mountContent(root / "content.zip"));
        requireBytes(files.readContent(sample), "height");
        requireOk(files.configureSaves(saves));
        requireBytes(files.readSave("profiles/player.bin"), "a");
    }
}
// End of Code Modification - Tests
}

int main(int argc, char* argv[])
{
    try {
        require(argc == 2, "Expected the native fixture root");
        verify(argv[1]);
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
