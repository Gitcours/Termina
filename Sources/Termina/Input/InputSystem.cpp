#include "InputSystem.hpp"

#include <GLFW/glfw3.h>
#include <cstring>
#include <cmath>

#include "ImGui/imgui.h"

namespace Termina {

    InputSystem* InputSystem::s_Instance = nullptr;

    InputSystem::InputSystem(GLFWwindow* window)
        : m_Window(window)
    {
        s_Instance = this;

        glfwSetKeyCallback(window, KeyCallbackGLFW);
        glfwSetMouseButtonCallback(window, MouseButtonCallbackGLFW);
        glfwSetCursorPosCallback(window, CursorPosCallbackGLFW);
        glfwSetScrollCallback(window, ScrollCallbackGLFW);

        double x, y;
        glfwGetCursorPos(window, &x, &y);
        m_MouseX = m_PrevMouseX = x;
        m_MouseY = m_PrevMouseY = y;
    }

    InputSystem::~InputSystem()
    {
        if (s_Instance == this)
            s_Instance = nullptr;
    }

    void InputSystem::PreUpdate(float deltaTime)
    {
        memcpy(m_KeyPrevious, m_KeyCurrent, sizeof(m_KeyCurrent));
        memcpy(m_MouseButtonPrevious, m_MouseButtonCurrent, sizeof(m_MouseButtonCurrent));
        memcpy(m_GamepadButtonPrevious, m_GamepadButtonCurrent, sizeof(m_GamepadButtonCurrent));

        m_PrevMouseX = m_MouseX;
        m_PrevMouseY = m_MouseY;
        m_MouseDeltaX = 0.0;
        m_MouseDeltaY = 0.0;
        m_ScrollX = m_ScrollAccumX;
        m_ScrollY = m_ScrollAccumY;
        m_ScrollAccumX = 0.0;
        m_ScrollAccumY = 0.0;

        glfwPollEvents();
    }

    void InputSystem::PostUpdate(float deltaTime)
    {
        PollGamepads();
    }

    void InputSystem::PollGamepads()
    {
        for (int32 i = 0; i < MaxGamepads; i++)
        {
            if (!glfwJoystickIsGamepad(GLFW_JOYSTICK_1 + i))
                continue;

            GLFWgamepadstate state;
            if (!glfwGetGamepadState(GLFW_JOYSTICK_1 + i, &state))
                continue;

            for (int32 b = 0; b < MaxGamepadButtons; b++)
                m_GamepadButtonCurrent[i][b] = state.buttons[b] == GLFW_PRESS;

            for (int32 a = 0; a < MaxGamepadAxes; a++)
                m_GamepadAxes[i][a] = state.axes[a];
        }
    }

    bool InputSystem::IsKeyPressed(Key key)
    {
        int32 k = static_cast<int32>(key);
        if (k < 0 || k >= MaxKeys) return false;
        return s_Instance->m_KeyCurrent[k] && !s_Instance->m_KeyPrevious[k];
    }

    bool InputSystem::IsKeyHeld(Key key)
    {
        int32 k = static_cast<int32>(key);
        if (k < 0 || k >= MaxKeys) return false;
        return s_Instance->m_KeyCurrent[k] && s_Instance->m_KeyPrevious[k];
    }

    bool InputSystem::IsKeyReleased(Key key)
    {
        int32 k = static_cast<int32>(key);
        if (k < 0 || k >= MaxKeys) return false;
        return !s_Instance->m_KeyCurrent[k] && s_Instance->m_KeyPrevious[k];
    }

    bool InputSystem::IsMouseButtonPressed(MouseButton button)
    {
        int32 b = static_cast<int32>(button);
        if (b < 0 || b >= MaxMouseButtons) return false;
        return s_Instance->m_MouseButtonCurrent[b] && !s_Instance->m_MouseButtonPrevious[b];
    }

    bool InputSystem::IsMouseButtonHeld(MouseButton button)
    {
        int32 b = static_cast<int32>(button);
        if (b < 0 || b >= MaxMouseButtons) return false;
        return s_Instance->m_MouseButtonCurrent[b] && s_Instance->m_MouseButtonPrevious[b];
    }

    bool InputSystem::IsMouseButtonReleased(MouseButton button)
    {
        int32 b = static_cast<int32>(button);
        if (b < 0 || b >= MaxMouseButtons) return false;
        return !s_Instance->m_MouseButtonCurrent[b] && s_Instance->m_MouseButtonPrevious[b];
    }

    glm::vec2 InputSystem::GetMousePosition()
    {
        return { static_cast<float>(s_Instance->m_MouseX), static_cast<float>(s_Instance->m_MouseY) };
    }

    glm::vec2 InputSystem::GetMouseDelta()
    {
        return { static_cast<float>(s_Instance->m_MouseDeltaX), static_cast<float>(s_Instance->m_MouseDeltaY) };
    }

    glm::vec2 InputSystem::GetScrollDelta()
    {
        return { static_cast<float>(s_Instance->m_ScrollX), static_cast<float>(s_Instance->m_ScrollY) };
    }

    void InputSystem::SetCursorVisible(bool visible)
    {
        glfwSetInputMode(s_Instance->m_Window, GLFW_CURSOR,
            visible ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_HIDDEN);
    }

    void InputSystem::SetCursorLocked(bool locked)
    {
        glfwSetInputMode(s_Instance->m_Window, GLFW_CURSOR,
            locked ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
    }

    bool InputSystem::IsGamepadConnected(int32 gamepadId)
    {
        return glfwJoystickIsGamepad(GLFW_JOYSTICK_1 + gamepadId);
    }

    bool InputSystem::IsGamepadButtonPressed(int32 gamepadId, GamepadButton button)
    {
        if (gamepadId < 0 || gamepadId >= MaxGamepads) return false;
        int32 b = static_cast<int32>(button);
        return s_Instance->m_GamepadButtonCurrent[gamepadId][b] &&
              !s_Instance->m_GamepadButtonPrevious[gamepadId][b];
    }

    bool InputSystem::IsGamepadButtonHeld(int32 gamepadId, GamepadButton button)
    {
        if (gamepadId < 0 || gamepadId >= MaxGamepads) return false;
        int32 b = static_cast<int32>(button);
        return s_Instance->m_GamepadButtonCurrent[gamepadId][b] &&
               s_Instance->m_GamepadButtonPrevious[gamepadId][b];
    }

    bool InputSystem::IsGamepadButtonReleased(int32 gamepadId, GamepadButton button)
    {
        if (gamepadId < 0 || gamepadId >= MaxGamepads) return false;
        int32 b = static_cast<int32>(button);
        return !s_Instance->m_GamepadButtonCurrent[gamepadId][b] &&
                s_Instance->m_GamepadButtonPrevious[gamepadId][b];
    }

    float InputSystem::GetGamepadAxis(int32 gamepadId, GamepadAxis axis, float deadzone)
    {
        if (gamepadId < 0 || gamepadId >= MaxGamepads) return 0.0f;
        int32 a = static_cast<int32>(axis);
        float value = s_Instance->m_GamepadAxes[gamepadId][a];
        return (std::abs(value) < deadzone) ? 0.0f : value;
    }

    void InputSystem::MapAction(const std::string& name, std::vector<ActionBinding> bindings)
    {
        m_ActionMap[name] = std::move(bindings);
    }

    void InputSystem::MapAxis(const std::string& name, std::vector<AxisBinding> bindings)
    {
        m_AxisMap[name] = std::move(bindings);
    }

    void InputSystem::UnmapAction(const std::string& name)
    {
        m_ActionMap.erase(name);
    }

    void InputSystem::UnmapAxis(const std::string& name)
    {
        m_AxisMap.erase(name);
    }

    bool InputSystem::IsActionPressed(const std::string& name)
    {
        auto it = s_Instance->m_ActionMap.find(name);
        if (it == s_Instance->m_ActionMap.end()) return false;
        for (const auto& binding : it->second)
        {
            switch (binding.BindingType)
            {
                case ActionBinding::Type::Key:
                    if (IsKeyPressed(binding.KeyCode)) return true;
                    break;
                case ActionBinding::Type::MouseButton:
                    if (IsMouseButtonPressed(binding.MouseBtn)) return true;
                    break;
                case ActionBinding::Type::GamepadButton:
                    if (IsGamepadButtonPressed(binding.GamepadId, binding.GamepadBtn)) return true;
                    break;
            }
        }
        return false;
    }

    bool InputSystem::IsActionHeld(const std::string& name)
    {
        auto it = s_Instance->m_ActionMap.find(name);
        if (it == s_Instance->m_ActionMap.end()) return false;
        for (const auto& binding : it->second)
        {
            switch (binding.BindingType)
            {
                case ActionBinding::Type::Key:
                    if (IsKeyHeld(binding.KeyCode)) return true;
                    break;
                case ActionBinding::Type::MouseButton:
                    if (IsMouseButtonHeld(binding.MouseBtn)) return true;
                    break;
                case ActionBinding::Type::GamepadButton:
                    if (IsGamepadButtonHeld(binding.GamepadId, binding.GamepadBtn)) return true;
                    break;
            }
        }
        return false;
    }

    bool InputSystem::IsActionReleased(const std::string& name)
    {
        auto it = s_Instance->m_ActionMap.find(name);
        if (it == s_Instance->m_ActionMap.end()) return false;
        for (const auto& binding : it->second)
        {
            switch (binding.BindingType)
            {
                case ActionBinding::Type::Key:
                    if (IsKeyReleased(binding.KeyCode)) return true;
                    break;
                case ActionBinding::Type::MouseButton:
                    if (IsMouseButtonReleased(binding.MouseBtn)) return true;
                    break;
                case ActionBinding::Type::GamepadButton:
                    if (IsGamepadButtonReleased(binding.GamepadId, binding.GamepadBtn)) return true;
                    break;
            }
        }
        return false;
    }

    float InputSystem::GetAxis(const std::string& name)
    {
        auto it = s_Instance->m_AxisMap.find(name);
        if (it == s_Instance->m_AxisMap.end()) return 0.0f;
        float result = 0.0f;
        for (const auto& binding : it->second)
        {
            switch (binding.BindingType)
            {
                case AxisBinding::Type::Key:
                    if (IsKeyHeld(binding.KeyCode) || IsKeyPressed(binding.KeyCode))
                        result += binding.Scale;
                    break;
                case AxisBinding::Type::MouseDeltaX:
                    result += static_cast<float>(s_Instance->m_MouseDeltaX) * binding.Scale;
                    break;
                case AxisBinding::Type::MouseDeltaY:
                    result += static_cast<float>(s_Instance->m_MouseDeltaY) * binding.Scale;
                    break;
                case AxisBinding::Type::ScrollX:
                    result += static_cast<float>(s_Instance->m_ScrollX) * binding.Scale;
                    break;
                case AxisBinding::Type::ScrollY:
                    result += static_cast<float>(s_Instance->m_ScrollY) * binding.Scale;
                    break;
                case AxisBinding::Type::GamepadAxis:
                    result += GetGamepadAxis(binding.GamepadId, binding.GamepadAxisCode) * binding.Scale;
                    break;
            }
        }
        return result;
    }

    void InputSystem::AddKeyCallback(KeyCallback callback)
    {
        m_KeyCallbacks.push_back(std::move(callback));
    }

    void InputSystem::AddMouseButtonCallback(MouseButtonCallback callback)
    {
        m_MouseButtonCallbacks.push_back(std::move(callback));
    }

    void InputSystem::KeyCallbackGLFW(GLFWwindow* window, int key, int scancode, int action, int mods)
    {
        if (key < 0 || key >= MaxKeys) return;
        bool pressed = action != GLFW_RELEASE;
        s_Instance->m_KeyCurrent[key] = pressed;
        for (auto& cb : s_Instance->m_KeyCallbacks)
            cb(static_cast<Key>(key), pressed);
    }

    void InputSystem::MouseButtonCallbackGLFW(GLFWwindow* window, int button, int action, int mods)
    {
        if (button < 0 || button >= MaxMouseButtons) return;
        bool pressed = action != GLFW_RELEASE;
        s_Instance->m_MouseButtonCurrent[button] = pressed;
        for (auto& cb : s_Instance->m_MouseButtonCallbacks)
            cb(static_cast<MouseButton>(button), pressed);
    }

    void InputSystem::CursorPosCallbackGLFW(GLFWwindow* window, double xpos, double ypos)
    {
        if (s_Instance->m_FirstMouse)
        {
            s_Instance->m_MouseX = xpos;
            s_Instance->m_MouseY = ypos;
            s_Instance->m_PrevMouseX = xpos;
            s_Instance->m_PrevMouseY = ypos;
            s_Instance->m_FirstMouse = false;
        }

        s_Instance->m_MouseDeltaX += xpos - s_Instance->m_MouseX;
        s_Instance->m_MouseDeltaY += ypos - s_Instance->m_MouseY;
        s_Instance->m_MouseX = xpos;
        s_Instance->m_MouseY = ypos;
    }

    void InputSystem::ScrollCallbackGLFW(GLFWwindow* window, double xoffset, double yoffset)
    {
        s_Instance->m_ScrollAccumX += xoffset;
        s_Instance->m_ScrollAccumY += yoffset;
    }

    static const char* GetKeyName(int32 keyCode)
    {
        switch (keyCode)
        {
            case 32:  return "Space";
            case 39:  return "'";
            case 44:  return ",";
            case 45:  return "-";
            case 46:  return ".";
            case 47:  return "/";
            case 48:  return "0"; case 49: return "1"; case 50: return "2";
            case 51:  return "3"; case 52: return "4"; case 53: return "5";
            case 54:  return "6"; case 55: return "7"; case 56: return "8";
            case 57:  return "9";
            case 59:  return ";";
            case 61:  return "=";
            case 65:  return "A"; case 66: return "B"; case 67: return "C";
            case 68:  return "D"; case 69: return "E"; case 70: return "F";
            case 71:  return "G"; case 72: return "H"; case 73: return "I";
            case 74:  return "J"; case 75: return "K"; case 76: return "L";
            case 77:  return "M"; case 78: return "N"; case 79: return "O";
            case 80:  return "P"; case 81: return "Q"; case 82: return "R";
            case 83:  return "S"; case 84: return "T"; case 85: return "U";
            case 86:  return "V"; case 87: return "W"; case 88: return "X";
            case 89:  return "Y"; case 90: return "Z";
            case 256: return "Escape";
            case 257: return "Enter";
            case 258: return "Tab";
            case 259: return "Backspace";
            case 260: return "Insert";
            case 261: return "Delete";
            case 262: return "Right";
            case 263: return "Left";
            case 264: return "Down";
            case 265: return "Up";
            case 266: return "PageUp";
            case 267: return "PageDown";
            case 268: return "Home";
            case 269: return "End";
            case 280: return "CapsLock";
            case 281: return "ScrollLock";
            case 282: return "NumLock";
            case 284: return "Pause";
            case 290: return "F1";  case 291: return "F2";  case 292: return "F3";
            case 293: return "F4";  case 294: return "F5";  case 295: return "F6";
            case 296: return "F7";  case 297: return "F8";  case 298: return "F9";
            case 299: return "F10"; case 300: return "F11"; case 301: return "F12";
            case 320: return "KP0"; case 321: return "KP1"; case 322: return "KP2";
            case 323: return "KP3"; case 324: return "KP4"; case 325: return "KP5";
            case 326: return "KP6"; case 327: return "KP7"; case 328: return "KP8";
            case 329: return "KP9";
            case 330: return "KP.";
            case 331: return "KP/"; case 332: return "KP*";
            case 333: return "KP-"; case 334: return "KP+"; case 335: return "KPEnter";
            case 340: return "LShift";   case 341: return "LCtrl";
            case 342: return "LAlt";     case 343: return "LSuper";
            case 344: return "RShift";   case 345: return "RCtrl";
            case 346: return "RAlt";     case 347: return "RSuper";
            case 348: return "Menu";
            default:  return nullptr;
        }
    }

    static const char* GetMouseButtonName(int32 button)
    {
        switch (button)
        {
            case 0: return "Left";
            case 1: return "Right";
            case 2: return "Middle";
            case 3: return "Button4";
            case 4: return "Button5";
            case 5: return "Button6";
            case 6: return "Button7";
            case 7: return "Button8";
            default: return "Unknown";
        }
    }

    static const char* GetGamepadButtonName(int32 button)
    {
        switch (button)
        {
            case 0:  return "A";
            case 1:  return "B";
            case 2:  return "X";
            case 3:  return "Y";
            case 4:  return "LBumper";
            case 5:  return "RBumper";
            case 6:  return "Back";
            case 7:  return "Start";
            case 8:  return "Guide";
            case 9:  return "LThumb";
            case 10: return "RThumb";
            case 11: return "DUp";
            case 12: return "DRight";
            case 13: return "DDown";
            case 14: return "DLeft";
            default: return "?";
        }
    }

    static const char* GetGamepadAxisName(int32 axis)
    {
        switch (axis)
        {
            case 0: return "LeftX";
            case 1: return "LeftY";
            case 2: return "RightX";
            case 3: return "RightY";
            case 4: return "LTrigger";
            case 5: return "RTrigger";
            default: return "?";
        }
    }

    void InputSystem::ShowDebugWindow(bool* open)
    {
        if (!ImGui::Begin("Input System", open))
        {
            ImGui::End();
            return;
        }

        if (ImGui::CollapsingHeader("Keyboard", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Indent();
            bool anyHeld = false;
            for (int32 k = 0; k < MaxKeys; k++)
            {
                if (!s_Instance->m_KeyCurrent[k]) continue;
                const char* name = GetKeyName(k);
                if (!name) continue;
                anyHeld = true;
                ImGui::Text("%s", name);
                ImGui::SameLine();
            }
            if (!anyHeld)
                ImGui::TextDisabled("none");
            else
                ImGui::NewLine();
            ImGui::Unindent();
        }

        if (ImGui::CollapsingHeader("Mouse", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Indent();
            ImGui::Text("Position:  %.1f, %.1f", s_Instance->m_MouseX, s_Instance->m_MouseY);
            ImGui::Text("Delta:     %.2f, %.2f", s_Instance->m_MouseDeltaX, s_Instance->m_MouseDeltaY);
            ImGui::Text("Scroll:    %.2f, %.2f", s_Instance->m_ScrollX, s_Instance->m_ScrollY);
            ImGui::Spacing();
            for (int32 b = 0; b < MaxMouseButtons; b++)
            {
                if (!s_Instance->m_MouseButtonCurrent[b]) continue;
                ImGui::Text("%s", GetMouseButtonName(b));
                ImGui::SameLine();
            }
            ImGui::NewLine();
            ImGui::Unindent();
        }

        if (ImGui::CollapsingHeader("Gamepads", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Indent();
            bool anyConnected = false;
            for (int32 i = 0; i < MaxGamepads; i++)
            {
                if (!glfwJoystickIsGamepad(GLFW_JOYSTICK_1 + i)) continue;
                anyConnected = true;

                const char* name = glfwGetGamepadName(GLFW_JOYSTICK_1 + i);
                ImGui::Text("Gamepad %d: %s", i, name ? name : "Unknown");
                ImGui::Indent();

                ImGui::Text("Buttons: ");
                ImGui::SameLine();
                bool anyBtn = false;
                for (int32 b = 0; b < MaxGamepadButtons; b++)
                {
                    if (!s_Instance->m_GamepadButtonCurrent[i][b]) continue;
                    anyBtn = true;
                    ImGui::Text("%s", GetGamepadButtonName(b));
                    ImGui::SameLine();
                }
                if (!anyBtn) ImGui::TextDisabled("none");
                else         ImGui::NewLine();

                for (int32 a = 0; a < MaxGamepadAxes; a++)
                {
                    float val = s_Instance->m_GamepadAxes[i][a];
                    float display = (std::abs(val) < 0.1f) ? 0.0f : val;
                    float fraction = (display + 1.0f) * 0.5f;
                    char overlay[32];
                    snprintf(overlay, sizeof(overlay), "%.2f", display);
                    ImGui::ProgressBar(fraction, ImVec2(-1, 0), overlay);
                    ImGui::SameLine(0, 4);
                    ImGui::TextDisabled("%s", GetGamepadAxisName(a));
                }

                ImGui::Unindent();
                ImGui::Spacing();
            }
            if (!anyConnected)
                ImGui::TextDisabled("No gamepads connected");
            ImGui::Unindent();
        }

        if (ImGui::CollapsingHeader("Action Map"))
        {
            ImGui::Indent();
            if (s_Instance->m_ActionMap.empty())
            {
                ImGui::TextDisabled("No actions mapped");
            }
            else
            {
                for (const auto& [name, bindings] : s_Instance->m_ActionMap)
                {
                    bool held    = IsActionHeld(name);
                    bool pressed = IsActionPressed(name);
                    if (held || pressed)
                        ImGui::Text("%-20s %s", name.c_str(), held ? "[HELD]" : "[PRESSED]");
                    else
                        ImGui::TextDisabled("%-20s", name.c_str());
                }
            }
            ImGui::Unindent();
        }

        if (ImGui::CollapsingHeader("Axis Map"))
        {
            ImGui::Indent();
            if (s_Instance->m_AxisMap.empty())
            {
                ImGui::TextDisabled("No axes mapped");
            }
            else
            {
                for (const auto& [name, bindings] : s_Instance->m_AxisMap)
                {
                    float val = GetAxis(name);
                    float fraction = (val + 1.0f) * 0.5f;
                    char overlay[64];
                    snprintf(overlay, sizeof(overlay), "%s  %.2f", name.c_str(), val);
                    ImGui::ProgressBar(fraction, ImVec2(-1, 0), overlay);
                }
            }
            ImGui::Unindent();
        }

        ImGui::End();
    }

}
