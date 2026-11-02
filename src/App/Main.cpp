#include "Core/Core.h"
#include <iostream>
#include <string_view>

// Code Modification - Application: Provide bounded, scriptable baseline diagnostics before window bring-up.
// Scope: command-line executable only. Performance: exits after one diagnostic; no background work. Toggle: --version skips logging.
int main(int argc, char* argv[])
{
    if (argc == 1) {
        CE::Core::logBuildInfo();
        std::cout << "0.0.1 build baseline ready. Interactive application shell is planned for 0.0.2.\n";
        return 0;
    }
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
            std::cout << "Usage: WiseEngine [--version|--build-info|--help]\n";
            return 0;
        }
    }
    std::cerr << "Invalid arguments. Use WiseEngine --help.\n";
    return 2;
}
// End of Code Modification - Application
