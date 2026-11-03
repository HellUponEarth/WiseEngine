#pragma once

#include <cstdint>
#include <memory>

namespace CE::Renderer {
// Code Modification - Rendering: Own the initial DX12 scene independently of SDL.
// Scope: one window and its immediate context on the calling thread. Performance: one indexed draw per frame.
// Toggle: application render mode and DX12 validation flag.
class Dx12Scene {
public:
    Dx12Scene();
    ~Dx12Scene();
    Dx12Scene(const Dx12Scene&) = delete;
    Dx12Scene& operator=(const Dx12Scene&) = delete;

    bool open(void* nativeWindow, std::uint32_t width, std::uint32_t height, bool validation);
    bool draw(std::uint32_t width, std::uint32_t height, float angle, bool verifyPixels = false);
    bool close();

private:
    struct State;
    std::unique_ptr<State> state_;
};
// End of Code Modification - Rendering
}
