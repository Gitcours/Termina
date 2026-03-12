#include "ScriptInput.hpp"

using namespace Termina;

namespace TerminaScript {
    bool Input::IsKeyPressed(Termina::Key key)
    {
        return InputSystem::IsKeyPressed(key);
    }

    bool Input::IsKeyHeld(Termina::Key key)
    {
        return InputSystem::IsKeyHeld(key);
    }

    bool Input::IsKeyReleased(Termina::Key key)
    {
        return InputSystem::IsKeyReleased(key);
    }

    bool Input::IsMouseButtonPressed(Termina::MouseButton button)  
    {
        return InputSystem::IsMouseButtonPressed(button);
    }

    bool Input::IsMouseButtonHeld(Termina::MouseButton button)
    {
        return InputSystem::IsMouseButtonHeld(button);
    }

    bool Input::IsMouseButtonReleased(Termina::MouseButton button)
    {
        return InputSystem::IsMouseButtonReleased(button);
    }

    glm::vec2 Input::GetMousePosition()
    {
        return InputSystem::GetMousePosition();
    }

    glm::vec2 Input::GetMouseDelta()
    {
        return InputSystem::GetMouseDelta();
    }

    glm::vec2 Input::GetScrollDelta()
    {
        return InputSystem::GetScrollDelta();
    }

    void Input::SetCursorVisible(bool visible)
    {
        InputSystem::SetCursorVisible(visible);
    }

    void Input::SetCursorLocked(bool locked)
    {
        InputSystem::SetCursorLocked(locked);
    }

    bool Input::IsGamepadConnected(int32 gamepadId)
    {
        return InputSystem::IsGamepadConnected(gamepadId);
    }

    bool Input::IsGamepadButtonHeld(int32 gamepadId, Termina::GamepadButton button)
    {
        return InputSystem::IsGamepadButtonHeld(gamepadId, button);
    }

    bool Input::IsGamepadButtonReleased(int32 gamepadId, Termina::GamepadButton button)
    {
        return InputSystem::IsGamepadButtonReleased(gamepadId, button);
    }

    float Input::GetGamepadAxis(int32 gamepadId, Termina::GamepadAxis axis, float deadzone)
    {
        return InputSystem::GetGamepadAxis(gamepadId, axis, deadzone);
    }
    
    bool Input::IsActionPressed(const std::string& name)
    {
        return InputSystem::IsActionPressed(name);
    }

    bool Input::IsActionHeld(const std::string& name)
    {
        return InputSystem::IsActionHeld(name);
    }
    
    bool Input::IsActionReleased(const std::string& name)
    {
        return InputSystem::IsActionReleased(name);
    }

    float Input::GetAxis(const std::string& name)
    {
        return InputSystem::GetAxis(name);
    }
}
