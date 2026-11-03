#include "ApplicationShell.h"
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
        if (option == "--help") {
            std::cout << "Usage: WiseEngine [--quiet-title|--version|--build-info|--help]\n"
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
