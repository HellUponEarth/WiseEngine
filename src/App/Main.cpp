#include "ApplicationShell.h"
#include "ElapsedClock.h"
#include "PlatformInfo.h"
#include "Core/Core.h"
#include <iostream>
#include <string_view>

// Code Modification - Application: Launch the interactive shell while keeping scriptable build queries.
// Scope: executable entry point. Performance: sleeps between events. Toggle: --quiet-title.
int main(int argc, char* argv[])
{
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
        if (option == "--help") {
            std::cout << "Usage: WiseEngine [--quiet-title|--version|--build-info|--platform-info|--help]\n"
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
