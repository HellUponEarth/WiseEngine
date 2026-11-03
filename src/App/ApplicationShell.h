#pragma once

#include "Input/Input.h"
#include <memory>
#include <string>

namespace CE::App {

// Code Modification - Application: Give the main thread explicit ownership of SDL video and its window.
// Scope: single-window application shell. Performance: bounded event batches. Toggle: titleDiagnostics.
class ApplicationShell {
public:
    ApplicationShell() = default;
    ~ApplicationShell();
    ApplicationShell(const ApplicationShell&) = delete;
    ApplicationShell& operator=(const ApplicationShell&) = delete;

    bool open(int width, int height, bool titleDiagnostics, bool hidden = false);
    bool pump(bool wait);
    void close() noexcept;
    bool running() const noexcept { return running_; }
    SDL_WindowID windowId() const noexcept { return windowId_; }
    const Input::InputSnapshot& input() const noexcept { return input_; }
    int width() const noexcept { return width_; }
    int height() const noexcept { return height_; }
    int pixelWidth() const noexcept { return pixelWidth_; }
    int pixelHeight() const noexcept { return pixelHeight_; }

private:
    bool dispatch(const SDL_Event& event);
    bool refreshSize();
    void refreshTitle();

    std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)> window_{nullptr, SDL_DestroyWindow};
    std::string baseTitle_;
    Input::InputSnapshot input_{};
    SDL_WindowID windowId_{};
    int width_{};
    int height_{};
    int pixelWidth_{};
    int pixelHeight_{};
    bool ownsVideo_{};
    bool running_{};
    bool titleDiagnostics_{};
};
// End of Code Modification - Application
}
