#include "App/ApplicationShell.h"
#include <iostream>
#include <stdexcept>
#include <string_view>

namespace {

// Code Modification - Tests: Exercise the same SDL lifecycle and event pump used by the executable.
// Scope: dummy video integration. Performance: test only. Toggle: WISEENGINE_BUILD_TESTS.
void require(bool success, std::string_view message)
{
    if (!success) {
        throw std::runtime_error(std::string(message));
    }
}

void pushKey(SDL_WindowID window, Uint32 type, SDL_Scancode scancode, bool repeat = false)
{
    SDL_Event event{};
    event.type = type;
    event.key.windowID = window;
    event.key.scancode = scancode;
    event.key.repeat = repeat;
    require(SDL_PushEvent(&event), SDL_GetError());
}

void pushWindow(SDL_WindowID window, Uint32 type)
{
    SDL_Event event{};
    event.type = type;
    event.window.windowID = window;
    require(SDL_PushEvent(&event), SDL_GetError());
}

void exerciseShell()
{
    CE::App::ApplicationShell shell;
    require(SDL_SetHintWithPriority(SDL_HINT_VIDEO_DRIVER, "wiseengine-invalid-driver", SDL_HINT_OVERRIDE),
        "select invalid driver");
    require(!shell.open(320, 240, false, true), "invalid video driver must fail");
    require(!shell.running() && SDL_WasInit(SDL_INIT_VIDEO) == 0, "failed startup must leave video inactive");
    require(SDL_SetHintWithPriority(SDL_HINT_VIDEO_DRIVER, "dummy", SDL_HINT_OVERRIDE), "select dummy driver");
    require(shell.open(320, 240, true, true), "restart after failed startup");
    require(shell.width() == 320 && shell.height() == 240 && shell.pixelWidth() > 0, "initial dimensions");
    require(shell.pump(false), "initial event batch");

    const auto id = shell.windowId();
    pushKey(id + 1, SDL_EVENT_KEY_DOWN, SDL_SCANCODE_B);
    pushKey(id, SDL_EVENT_KEY_DOWN, SDL_SCANCODE_A);
    pushKey(id, SDL_EVENT_KEY_UP, SDL_SCANCODE_A);
    require(shell.pump(true), "key event batch");
    auto a = shell.input().keys[SDL_SCANCODE_A];
    require(a.pressed && a.released && !a.held, "same-batch tap keeps both edges");
    require(!shell.input().keys[SDL_SCANCODE_B].held, "foreign window events are ignored");
    pushKey(id, SDL_EVENT_KEY_DOWN, SDL_SCANCODE_A);
    require(shell.pump(true), "key down");
    pushKey(id, SDL_EVENT_KEY_DOWN, SDL_SCANCODE_A, true);
    require(shell.pump(true), "key repeat");
    a = shell.input().keys[SDL_SCANCODE_A];
    require(a.held && !a.pressed && !a.released, "repeat preserves hold without edges");

    SDL_Event motion{};
    motion.type = SDL_EVENT_MOUSE_MOTION;
    motion.motion.windowID = id;
    motion.motion.x = 12.0f;
    motion.motion.y = 24.0f;
    motion.motion.xrel = 3.0f;
    require(SDL_PushEvent(&motion) && SDL_PushEvent(&motion), "queue mouse motion");
    SDL_Event button{};
    button.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
    button.button.windowID = id;
    button.button.button = SDL_BUTTON_LEFT;
    button.button.x = motion.motion.x;
    button.button.y = motion.motion.y;
    require(SDL_PushEvent(&button), "queue mouse button");
    SDL_Event wheel{};
    wheel.type = SDL_EVENT_MOUSE_WHEEL;
    wheel.wheel.windowID = id;
    wheel.wheel.y = 2.0f;
    wheel.wheel.direction = SDL_MOUSEWHEEL_FLIPPED;
    require(SDL_PushEvent(&wheel), "queue wheel");
    require(shell.pump(true), "mouse event batch");
    require(shell.input().mouseDeltaX == 6.0f && shell.input().wheelY == -2.0f,
        "mouse delta accumulation and wheel direction");
    require(shell.input().mouseButtons[SDL_BUTTON_LEFT].pressed, "mouse button press");
    pushWindow(id, SDL_EVENT_WINDOW_FOCUS_LOST);
    require(shell.pump(true), "focus loss");
    require(shell.input().keys[SDL_SCANCODE_A].released && !shell.input().keys[SDL_SCANCODE_A].held,
        "focus loss clears keyboard hold");
    require(shell.input().mouseButtons[SDL_BUTTON_LEFT].released && shell.input().wheelY == 0.0f,
        "focus loss clears button hold and batch resets wheel");

    require(SDL_SetWindowSize(SDL_GetWindowFromID(id), 400, 300), SDL_GetError());
    pushWindow(id, SDL_EVENT_WINDOW_RESIZED);
    require(shell.pump(true), "resize");
    require(shell.width() == 400 && shell.height() == 300 && shell.pixelWidth() == 400, "updated dimensions");
    pushWindow(id, SDL_EVENT_WINDOW_CLOSE_REQUESTED);
    require(shell.pump(true) && !shell.running(), "window close");
    shell.close();
    shell.close();
    require(SDL_GetWindowFromID(id) == nullptr && SDL_WasInit(SDL_INIT_VIDEO) == 0, "idempotent teardown");

    require(shell.open(320, 240, false, true), "open after close");
    pushKey(shell.windowId(), SDL_EVENT_KEY_DOWN, SDL_SCANCODE_ESCAPE);
    require(shell.pump(true) && !shell.running(), "Escape exits");
    shell.close();
    require(shell.open(320, 240, false, true), "open before global quit");
    SDL_Event quit{};
    quit.type = SDL_EVENT_QUIT;
    require(SDL_PushEvent(&quit) && shell.pump(true) && !shell.running(), "global quit");
}
// End of Code Modification - Tests
}

int main()
{
    try {
        exerciseShell();
        if (SDL_WasInit(SDL_INIT_VIDEO) != 0) {
            throw std::runtime_error("Destructor did not release SDL video");
        }
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
