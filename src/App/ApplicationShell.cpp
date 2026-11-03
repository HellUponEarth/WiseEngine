#include "ApplicationShell.h"
#include "Core/Core.h"
#define SDL_MAIN_HANDLED
#include <SDL3/SDL_main.h>
#include <spdlog/spdlog.h>

namespace CE::App {

// Code Modification - Application: Unwind failed startup and normal shutdown in reverse ownership order.
// Scope: SDL video and window only. Performance: blocks when idle, at most 256 events per batch.
// Toggle: --quiet-title avoids formatting and updating diagnostic window titles.
ApplicationShell::~ApplicationShell()
{
    close();
}

bool ApplicationShell::open(int width, int height, bool titleDiagnostics, bool hidden)
{
    if (ownsVideo_) {
        spdlog::error("Application shell is already open");
        return false;
    }

    baseTitle_ = fmt::format("WiseEngine {}", Core::getBuildInfo().version);
    SDL_SetMainReady();
    if (!SDL_InitSubSystem(SDL_INIT_VIDEO)) {
        spdlog::error("SDL video initialization failed: {}", SDL_GetError());
        return false;
    }
    ownsVideo_ = true;
    SDL_WindowFlags flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY;
    if (hidden) {
        flags |= SDL_WINDOW_HIDDEN;
    }
    window_.reset(SDL_CreateWindow(baseTitle_.c_str(), width, height, flags));
    if (!window_) {
        spdlog::error("SDL window creation failed: {}", SDL_GetError());
        close();
        return false;
    }

    windowId_ = SDL_GetWindowID(window_.get());
    if (!refreshSize()) {
        close();
        return false;
    }
    titleDiagnostics_ = titleDiagnostics;
    running_ = true;
    if (titleDiagnostics_) {
        refreshTitle();
    }
    spdlog::info("SDL window opened at {}x{} ({}x{} pixels)", width_, height_, pixelWidth_, pixelHeight_);
    return true;
}

void ApplicationShell::close() noexcept
{
    running_ = false;
    window_.reset();
    windowId_ = 0;
    input_ = {};
    width_ = height_ = pixelWidth_ = pixelHeight_ = 0;
    if (ownsVideo_) {
        SDL_QuitSubSystem(SDL_INIT_VIDEO);
        ownsVideo_ = false;
    }
}

bool ApplicationShell::pump(bool wait)
{
    if (!running_) {
        return true;
    }
    input_.beginBatch();
    SDL_Event event{};
    if (wait) {
        if (!SDL_WaitEvent(&event)) {
            spdlog::error("SDL event wait failed: {}", SDL_GetError());
            running_ = false;
            return false;
        }
    } else if (!SDL_PollEvent(&event)) {
        return true;
    }

    bool titleChanged = false;
    constexpr int maxEventsPerBatch = 256;
    for (int count = 0; ; ++count) {
        titleChanged |= dispatch(event);
        if (!running_ || count + 1 == maxEventsPerBatch || !SDL_PollEvent(&event)) {
            break;
        }
    }
    if (running_ && titleDiagnostics_ && titleChanged) {
        refreshTitle();
    }
    return true;
}

bool ApplicationShell::dispatch(const SDL_Event& event)
{
    if (event.type == SDL_EVENT_QUIT) {
        running_ = false;
        return false;
    }

    // Read the window ID from the active union member before updating this window's snapshot.
    SDL_WindowID source{};
    switch (event.type) {
    case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
    case SDL_EVENT_WINDOW_RESIZED:
    case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
    case SDL_EVENT_WINDOW_FOCUS_LOST:
        source = event.window.windowID;
        break;
    case SDL_EVENT_KEY_DOWN:
    case SDL_EVENT_KEY_UP:
        source = event.key.windowID;
        break;
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    case SDL_EVENT_MOUSE_BUTTON_UP:
        source = event.button.windowID;
        break;
    case SDL_EVENT_MOUSE_MOTION:
        source = event.motion.windowID;
        break;
    case SDL_EVENT_MOUSE_WHEEL:
        source = event.wheel.windowID;
        break;
    default:
        return false;
    }
    if (source != windowId_) {
        return false;
    }
    if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
        running_ = false;
        return false;
    }
    if (event.type == SDL_EVENT_WINDOW_RESIZED || event.type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED) {
        return refreshSize();
    }
    const bool changed = input_.apply(event);
    if (event.type == SDL_EVENT_KEY_DOWN && event.key.scancode == SDL_SCANCODE_ESCAPE && !event.key.repeat) {
        running_ = false;
    }
    return changed;
}

bool ApplicationShell::refreshSize()
{
    int width{}, height{}, pixelWidth{}, pixelHeight{};
    if (!SDL_GetWindowSize(window_.get(), &width, &height)
        || !SDL_GetWindowSizeInPixels(window_.get(), &pixelWidth, &pixelHeight)) {
        spdlog::error("SDL window size query failed: {}", SDL_GetError());
        return false;
    }
    width_ = width;
    height_ = height;
    pixelWidth_ = pixelWidth;
    pixelHeight_ = pixelHeight;
    return true;
}

void ApplicationShell::refreshTitle()
{
    const char* key = input_.lastKey == SDL_SCANCODE_UNKNOWN ? "-" : SDL_GetScancodeName(input_.lastKey);
    const bool held = input_.keys[input_.lastKey].held;
    const auto title = fmt::format("{} | {}x{} | Key {} {} | Mouse {:.0f},{:.0f} {}",
        baseTitle_, width_, height_, key, held ? "down" : "up", input_.mouseX, input_.mouseY,
        input_.mouseButtons[SDL_BUTTON_LEFT].held ? "left down" : "");
    if (!SDL_SetWindowTitle(window_.get(), title.c_str())) {
        spdlog::warn("SDL window title update failed: {}", SDL_GetError());
        titleDiagnostics_ = false;
    }
}
// End of Code Modification - Application
}
