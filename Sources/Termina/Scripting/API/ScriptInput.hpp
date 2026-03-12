#pragma once

#include <Termina/Input/InputSystem.hpp>

namespace TerminaScript {
    class Input
    {
    public:
        static bool      IsKeyPressed(Termina::Key key);
        static bool      IsKeyHeld(Termina::Key key);
        static bool      IsKeyReleased(Termina::Key key);
        static bool      IsMouseButtonPressed(Termina::MouseButton button);
        static bool      IsMouseButtonHeld(Termina::MouseButton button);
        static bool      IsMouseButtonReleased(Termina::MouseButton button);
        static glm::vec2 GetMousePosition();
        static glm::vec2 GetMouseDelta();
        static glm::vec2 GetScrollDelta();
        static void      SetCursorVisible(bool visible);
        static void      SetCursorLocked(bool locked);
        static bool      IsGamepadConnected(int32 gamepadId);
        static bool      IsGamepadButtonPressed(int32 gamepadId, Termina::GamepadButton button);
        static bool      IsGamepadButtonHeld(int32 gamepadId, Termina::GamepadButton button);
        static bool      IsGamepadButtonReleased(int32 gamepadId, Termina::GamepadButton button);
        static float     GetGamepadAxis(int32 gamepadId, Termina::GamepadAxis axis, float deadzone = 0.1f);
        static bool      IsActionPressed(const std::string& name);
        static bool      IsActionHeld(const std::string& name);
        static bool      IsActionReleased(const std::string& name);
        static float     GetAxis(const std::string& name);
    };
}

