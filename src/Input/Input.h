#pragma once

#include <SDL3/SDL.h>
#include <array>

namespace CE::Input {

// Code Modification - Input: Preserve held state and both edges when events arrive in one batch.
// Scope: main-thread snapshot. Performance: fixed storage, reset once per event batch. Toggle: none.
struct ButtonState {
    bool held{};
    bool pressed{};
    bool released{};
};

struct InputSnapshot {
    std::array<ButtonState, SDL_SCANCODE_COUNT> keys{};
    std::array<ButtonState, 6> mouseButtons{}; // SDL button indices are one-based.
    SDL_Scancode lastKey{SDL_SCANCODE_UNKNOWN};
    float mouseX{};
    float mouseY{};
    float mouseDeltaX{};
    float mouseDeltaY{};
    float wheelX{};
    float wheelY{};

    void beginBatch() noexcept;
    bool apply(const SDL_Event& event) noexcept;
};
// End of Code Modification - Input

}
