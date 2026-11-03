#include "ApplicationShell.h"
#include "ElapsedClock.h"
#include "PlatformInfo.h"
#include "AssetManager/FileSystem.h"
#include "Core/Core.h"
#include "Jobs/JobSystem.h"

#include <exception>
#include <iostream>
#include <string_view>

// Code Modification - Jobs: Exercise both execution lanes without starting SDL video.
// Scope: headless CLI smoke. Performance: two worker threads for the duration of the probe. Toggle: --job-smoke.
namespace {
int runJobSmoke()
{
    try {
        using CE::Jobs::JobBytes;
        using CE::Jobs::SubmitResult;
        CE::Jobs::JobSystem jobs(1);
        const auto cpu = jobs.submitCpu({{{1, [] { return JobBytes{std::byte{1}}; }}, {}}});
        const auto io = jobs.submitIo({2, [] { return JobBytes{std::byte{2}}; }});
        jobs.shutdown();

        bool cpuComplete = false;
        bool ioComplete = false;
        while (const auto completion = jobs.poll()) {
            if (completion->error) {
                std::rethrow_exception(completion->error);
            }
            cpuComplete = cpuComplete || (completion->tag == 1 && completion->bytes == JobBytes{std::byte{1}});
            ioComplete = ioComplete || (completion->tag == 2 && completion->bytes == JobBytes{std::byte{2}});
        }
        if (cpu == SubmitResult::Accepted && io == SubmitResult::Accepted && cpuComplete && ioComplete) {
            std::cout << "CPU and I/O jobs completed.\n";
            return 0;
        }
        std::cerr << "Job smoke did not return both expected results.\n";
    } catch (const std::exception& error) {
        std::cerr << "Job smoke failed: " << error.what() << '\n';
    }
    return 1;
}
}
// End of Code Modification - Jobs

// Code Modification - File System: Inspect one loose or archived resource from the command line.
// Scope: headless native tooling. Performance: bounded synchronous read on demand. Toggle: --fs-read.
namespace {
int runFileRead(const char* source, std::string_view path)
{
    try {
        using namespace CE::AssetManager;
        FileSystem files;
        if (const auto error = files.mountContent(source)) {
            std::cerr << error->message << '\n';
            return 1;
        }
        const auto result = files.readContent(path);
        if (const auto* error = std::get_if<FileError>(&result)) {
            std::cerr << error->message << '\n';
            return 1;
        }
        std::cout << "bytes=" << std::get<FileBytes>(result).size() << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Filesystem probe failed: " << error.what() << '\n';
        return 1;
    }
}
}
// End of Code Modification - File System

// Code Modification - Application: Launch the interactive shell while keeping scriptable build queries.
// Scope: executable entry point. Performance: sleeps between events. Toggle: --quiet-title.
int main(int argc, char* argv[])
{
    if (argc == 4 && std::string_view(argv[1]) == "--fs-read") {
        return runFileRead(argv[2], argv[3]);
    }
    bool titleDiagnostics = true;
    if (argc == 2) {
        const std::string_view option(argv[1]);
        const auto info = CE::Core::getBuildInfo();
        if (option == "--version") {
            std::cout << "WiseEngine " << info.version << '\n';
            return 0;
        }
        if (option == "--build-info") {
            std::cout << "version=" << info.version << "\nconfiguration=" << info.configuration
                << "\ncompiler=" << info.compiler << "\npointer_bits=" << info.pointerBits
                << "\ncpp_standard=" << info.cppStandard << '\n';
            return 0;
        }
        // Code Modification - Platform: Allow build and timer probes on headless systems.
        // Scope: CLI diagnostics. Performance: one SDL frequency query on demand. Toggle: --platform-info.
        if (option == "--platform-info") {
            const auto platform = CE::App::getPlatformInfo();
            const auto frequency = CE::App::ElapsedClock::frequency();
            std::cout << "os=" << platform.os << "\narchitecture=" << platform.architecture
                << "\ncompiler=" << platform.compiler << "\nbyte_order=" << platform.byteOrder
                << "\npointer_bits=" << platform.pointerBits << "\nperformance_counter_hz=" << frequency << '\n';
            return frequency == 0 ? 1 : 0;
        }
        // End of Code Modification - Platform
        if (option == "--job-smoke") {
            return runJobSmoke();
        }
        if (option == "--help") {
            std::cout << "Usage: WiseEngine [--quiet-title|--version|--build-info|--platform-info|--job-smoke|--help]\n"
                << "       WiseEngine --fs-read <native-source> <logical-file>\n"
                << "Resize the window, use the keyboard or mouse, and press Escape to exit.\n";
            return 0;
        }
        if (option == "--quiet-title") {
            titleDiagnostics = false;
        }
    }
    if (argc != 1 && titleDiagnostics) {
        std::cerr << "Invalid arguments. Use WiseEngine --help.\n";
        return 2;
    }

    CE::Core::logBuildInfo();
    CE::App::ApplicationShell shell;
    if (!shell.open(960, 540, titleDiagnostics)) {
        return 1;
    }
    while (shell.running()) {
        if (!shell.pump(true)) {
            return 1;
        }
    }
    shell.close();
    std::cout << "WiseEngine shutdown complete.\n";
    return 0;
}
// End of Code Modification - Application
