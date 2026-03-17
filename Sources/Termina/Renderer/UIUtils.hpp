#pragma once

#include <ImGui/imgui.h>
#include <Termina/Asset/AssetHandle.hpp>
#include <string>
#include <functional>

namespace Termina {
    class Actor;

    /// Utility functions for UI rendering.
    class UIUtils
    {
    public:
        static void Setup();
        static void DrawStylizedText(const char* text, ImVec2 offset = ImVec2(8, 4));

        // Stylized context — widgets inside push/pop render with mixed Fraktur capital + Playfair body
        static void PushStylized();
        static void PopStylized();
        static bool IsStylized();

        // Window wrapper
        static bool BeginEditorWindow(const char* title, bool* open = nullptr, ImGuiWindowFlags flags = 0);
        static void EndEditorWindow();

        // Stylized widget wrappers (pass-through when not in stylized context)
        static bool Button(const char* label, ImVec2 size = ImVec2(0, 0));
        static bool CollapsingHeader(const char* label, ImGuiTreeNodeFlags flags = 0);
        static bool TreeNode(const char* label);
        static bool TreeNodeEx(const char* label, ImGuiTreeNodeFlags flags = 0);
        static void TreePop();
        static bool BeginMenuBar();
        static void EndMenuBar();
        static bool BeginMenu(const char* label, bool enabled = true);
        static void EndMenu();
        static bool MenuItem(const char* label, const char* shortcut = nullptr, bool selected = false, bool enabled = true);

        static void SetDarkTheme(bool dark);
        static bool IsDarkTheme();

        // Drag and drop — assets (path-based)
        static void AssetPickerSource(const std::string& path);
        /// Renders a drop-target button for a prefab (.trp) asset. Returns true if the path changed.
        static bool PrefabPicker(std::string& path);
        /// Renders a drop-target button for a world (.trw) asset. Returns true if the path changed.
        static bool ScenePicker(std::string& path);

        template<typename T>
        static bool TryReceiveAsset(const AssetHandle<T>& handle, const std::function<void(const std::string&)>& callback)
        {
            return TryReceiveAssetImpl(handle.GetPath(), callback);
        }

        // Drag and drop — actors (pointer-based)
        static void ActorPickerSource(Actor* actor);
        static bool TryReceiveActor(Actor* current, const std::function<void(Actor*)>& callback);

        // Low-level drag and drop (no UI)
        static bool AcceptActor(const std::function<void(Actor*)>& callback);
        static bool AcceptAsset(const std::function<void(const std::string&)>& callback);

    private:
        static void SetTheme();
        static bool TryReceiveAssetImpl(const std::string& currentPath, const std::function<void(const std::string&)>& callback);

        static struct Data {
            ImFont* RegularFont;
            ImFont* DarkRegularFont;
            ImFont* CapitalFont;
            int StylizedDepth = 0;
            bool IsDark = true;
        } sData;
    };
}
