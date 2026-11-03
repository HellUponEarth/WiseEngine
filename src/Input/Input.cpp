#include "Input.h"

namespace CE::Input {
namespace {

// Code Modification - Input: Record transitions without turning keyboard repeat into a second press.
// Scope: keyboard and mouse events. Performance: constant work per transition. Toggle: none.
void setButton(ButtonState& button, bool held) noexcept
{
    if (button.held == held) {
        return;
    }
    button.held = held;
    button.pressed |= held;
    button.released |= !held;
}

void clearEdges(auto& buttons) noexcept
{
    for (auto& button : buttons) {
        button.pressed = false;
        button.released = false;
    }
}

}

void InputSnapshot::beginBatch() noexcept
{
    clearEdges(keys);
    clearEdges(mouseButtons);
    mouseDeltaX = mouseDeltaY = wheelX = wheelY = 0.0f;
}

bool InputSnapshot::apply(const SDL_Event& event) noexcept
{
    switch (event.type) {
    case SDL_EVENT_KEY_DOWN:
    case SDL_EVENT_KEY_UP: {
        const int key = event.key.scancode;
        if (key <= SDL_SCANCODE_UNKNOWN || key >= SDL_SCANCODE_COUNT || event.key.repeat) {
            return false;
        }
        setButton(keys[key], event.type == SDL_EVENT_KEY_DOWN);
        lastKey = event.key.scancode;
        return true;
    }
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    case SDL_EVENT_MOUSE_BUTTON_UP:
        if (event.button.button == 0 || event.button.button >= mouseButtons.size()) {
            return false;
        }
        mouseX = event.button.x;
        mouseY = event.button.y;
        setButton(mouseButtons[event.button.button], event.type == SDL_EVENT_MOUSE_BUTTON_DOWN);
        return true;
    case SDL_EVENT_MOUSE_MOTION:
        mouseX = event.motion.x;
        mouseY = event.motion.y;
        mouseDeltaX += event.motion.xrel;
        mouseDeltaY += event.motion.yrel;
        return true;
    case SDL_EVENT_MOUSE_WHEEL: {
        const float sign = event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED ? -1.0f : 1.0f;
        wheelX += sign * event.wheel.x;
        wheelY += sign * event.wheel.y;
        return true;
    }
    case SDL_EVENT_WINDOW_FOCUS_LOST:
        // Release held controls because the window may miss their physical release.
        for (auto& key : keys) {
            setButton(key, false);
        }
        for (auto& button : mouseButtons) {
            setButton(button, false);
        }
        return true;
    default:
        return false;
    }
}
// End of Code Modification - Input
}
