#include "UIUtils.hpp"
#include "Core/Logger.hpp"
#include "ImGui/imgui.h"
#include "World/Actor.hpp"
#include <string>
#include <filesystem>

namespace Termina {
    UIUtils::Data UIUtils::sData;

    void UIUtils::Setup()
    {
        ImGuiIO& io = ImGui::GetIO();
        sData.RegularFont = io.Fonts->AddFontFromFileTTF("__TERMINA__/FONTS/PlayfairDisplay-Regular.ttf", 20.0f);
        sData.CapitalFont = io.Fonts->AddFontFromFileTTF("__TERMINA__/FONTS/PlayfairDisplay-Regular.ttf", 24.0f);

        SetTheme();
    }

    void UIUtils::DrawStylizedText(const char* text, ImVec2 offset)
    {
        if (!text || text[0] == '\0') return;

        ImGui::PushFont(sData.CapitalFont, sData.CapitalFont->LegacySize);
        ImGui::Text("%c", text[0]);
        ImGui::PopFont();

        if (text[1] != '\0')
        {
            ImGui::SameLine(0, 0); // no spacing
            ImGui::SetCursorPosY(ImGui::GetCursorPosY());
            ImGui::PushFont(sData.RegularFont, sData.RegularFont->LegacySize);
            ImGui::Text("%s", text + 1);
            ImGui::PopFont();
        }
    }

    bool UIUtils::BeginEditorWindow(const char* title, bool* open, ImGuiWindowFlags flags)
    {
        ImGui::PushFont(sData.RegularFont, sData.RegularFont->LegacySize);

        bool visible = ImGui::Begin(title, open, flags); // keep the real title bar
        float regularFontSize = 18.0f * ImGui::GetIO().FontGlobalScale;
        float capitalFontSize = 20.0f * ImGui::GetIO().FontGlobalScale;

        if (visible)
        {
            float titleBarHeight = ImGui::GetFrameHeight();
            ImVec2 windowPos = ImGui::GetWindowPos();
            float windowWidth = ImGui::GetWindowWidth();

            ImDrawList* dl = ImGui::GetWindowDrawList();

            dl->PushClipRect(
                windowPos,
                ImVec2(windowPos.x + windowWidth, windowPos.y + ImGui::GetWindowHeight()),
                false // false = don't intersect with current cliprect, replace it
            );

            dl->AddRectFilled(
                windowPos,
                ImVec2(windowPos.x + windowWidth, windowPos.y + titleBarHeight),
                IM_COL32(255, 255, 255, 255)
            );

            dl->AddLine(
                ImVec2(windowPos.x, windowPos.y + titleBarHeight),
                ImVec2(windowPos.x + windowWidth, windowPos.y + titleBarHeight),
                IM_COL32(0, 0, 0, 255), 1.0f
            );

            // Draw text directly via drawlist, bypassing cursor/clip issues
            ImVec2 textPos = ImVec2(windowPos.x + 8, windowPos.y + ImGui::GetStyle().FramePadding.y);

            // Capital letter with Unifraktur
            int cmdCountBefore = dl->CmdBuffer.Size;
            dl->AddText(sData.CapitalFont, capitalFontSize,
                textPos,
                IM_COL32(0, 0, 0, 255),
                title, title + 1
            );

            // Rest with IM Fell — advance x by the capital glyph width
            float capWidth = sData.CapitalFont->CalcTextSizeA(capitalFontSize, FLT_MAX, 0, title, title + 1).x;
            dl->AddText(sData.RegularFont, regularFontSize,
                ImVec2(textPos.x + (capWidth + 2), textPos.y),
                IM_COL32(0, 0, 0, 255),
                title + 1
            );


            dl->PopClipRect();

            ImGui::SetCursorPosY(titleBarHeight + ImGui::GetStyle().ItemSpacing.y);
            ImGui::Dummy(ImVec2(0, 0));
        }

        return visible;
    }

    void UIUtils::EndEditorWindow()
    {
        ImGui::End();
        ImGui::PopFont(); // matches PushFont in BeginEditorWindow
    }

    void UIUtils::PushStylized()
    {
        sData.StylizedDepth++;
        if (sData.StylizedDepth == 1)
            ImGui::SetStylizedCapitalFont(sData.CapitalFont);
    }

    void UIUtils::PopStylized()
    {
        sData.StylizedDepth--;
        if (sData.StylizedDepth == 0)
            ImGui::SetStylizedCapitalFont(nullptr);
    }

    bool UIUtils::IsStylized()
    {
        return sData.StylizedDepth > 0;
    }

    bool UIUtils::Button(const char* label, ImVec2 size)               { return ImGui::Button(label, size); }
    bool UIUtils::CollapsingHeader(const char* label, ImGuiTreeNodeFlags flags) { return ImGui::CollapsingHeader(label, flags); }
    bool UIUtils::TreeNode(const char* label)                           { return ImGui::TreeNode(label); }
    bool UIUtils::TreeNodeEx(const char* label, ImGuiTreeNodeFlags flags) { return ImGui::TreeNodeEx(label, flags); }
    void UIUtils::TreePop()                                             { ImGui::TreePop(); }
    bool UIUtils::BeginMenuBar()                                        { return ImGui::BeginMenuBar(); }
    void UIUtils::EndMenuBar()                                          { ImGui::EndMenuBar(); }
    bool UIUtils::BeginMenu(const char* label, bool enabled)            { return ImGui::BeginMenu(label, enabled); }
    void UIUtils::EndMenu()                                             { ImGui::EndMenu(); }
    bool UIUtils::MenuItem(const char* label, const char* shortcut, bool selected, bool enabled) { return ImGui::MenuItem(label, shortcut, selected, enabled); }

    void UIUtils::AssetPickerSource(const std::string& path)
    {
        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
        {
            ImGui::SetDragDropPayload("ASSET_PATH", path.c_str(), path.size() + 1);
            ImGui::Text("%s", path.c_str());
            ImGui::EndDragDropSource();
        }
    }

    bool UIUtils::PrefabPicker(std::string& path)
    {
        const std::string label = path.empty()
            ? "No Prefab"
            : std::filesystem::path(path).filename().string();

        ImVec2 size = ImVec2(ImGui::GetContentRegionAvail().x, 0);
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_FrameBg));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetStyleColorVec4(ImGuiCol_FrameBgHovered));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImGui::GetStyleColorVec4(ImGuiCol_FrameBgActive));
        ImGui::Button(label.c_str(), size);
        ImGui::PopStyleColor(3);

        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_PATH"))
            {
                const char* dropped = static_cast<const char*>(payload->Data);
                if (dropped && dropped[0] != '\0' &&
                    std::filesystem::path(dropped).extension() == ".trp")
                {
                    path = dropped;
                    ImGui::EndDragDropTarget();
                    return true;
                }
            }
            ImGui::EndDragDropTarget();
        }
        return false;
    }

    bool UIUtils::ScenePicker(std::string& path)
    {
        const std::string label = path.empty()
            ? "No Scene"
            : std::filesystem::path(path).filename().string();

        ImVec2 size = ImVec2(ImGui::GetContentRegionAvail().x, 0);
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_FrameBg));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetStyleColorVec4(ImGuiCol_FrameBgHovered));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImGui::GetStyleColorVec4(ImGuiCol_FrameBgActive));
        ImGui::Button(label.c_str(), size);
        ImGui::PopStyleColor(3);

        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_PATH"))
            {
                const char* dropped = static_cast<const char*>(payload->Data);
                if (dropped && dropped[0] != '\0' &&
                    std::filesystem::path(dropped).extension() == ".trw")
                {
                    path = dropped;
                    ImGui::EndDragDropTarget();
                    return true;
                }
            }
            ImGui::EndDragDropTarget();
        }
        return false;
    }

    bool UIUtils::TryReceiveAssetImpl(const std::string& current, const std::function<void(const std::string&)>& callback)
    {
        const std::string label = current.empty()
            ? "No Asset"
            : std::filesystem::path(current).filename().string();

        ImVec2 size = ImVec2(ImGui::GetContentRegionAvail().x, 0);
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_FrameBg));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetStyleColorVec4(ImGuiCol_FrameBgHovered));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImGui::GetStyleColorVec4(ImGuiCol_FrameBgActive));
        ImGui::Button(label.c_str(), size);
        ImGui::PopStyleColor(3);

        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_PATH"))
            {
                const char* path = static_cast<const char*>(payload->Data);
                if (path && path[0] != '\0')
                    callback(path);
                ImGui::EndDragDropTarget();
                return true;
            }
            ImGui::EndDragDropTarget();
        }
        return false;
    }

    void UIUtils::ActorPickerSource(Actor* actor)
    {
        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
        {
            ImGui::SetDragDropPayload("ACTOR_PTR", &actor, sizeof(Actor*));
            ImGui::EndDragDropSource();
        }
    }

    bool UIUtils::TryReceiveActor(Actor* current, const std::function<void(Actor*)>& callback)
    {
        const std::string label = current ? current->GetName() : "No Actor";

        ImVec2 size = ImVec2(ImGui::GetContentRegionAvail().x, 0);
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_FrameBg));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetStyleColorVec4(ImGuiCol_FrameBgHovered));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImGui::GetStyleColorVec4(ImGuiCol_FrameBgActive));
        ImGui::Button(label.c_str(), size);
        ImGui::PopStyleColor(3);

        return AcceptActor(callback);
    }

    bool UIUtils::AcceptActor(const std::function<void(Actor*)>& callback)
    {
        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ACTOR_PTR"))
            {
                Actor* actor = *static_cast<Actor**>(payload->Data);
                if (actor)
                    callback(actor);
                ImGui::EndDragDropTarget();
                return true;
            }
            ImGui::EndDragDropTarget();
        }
        return false;
    }

    bool UIUtils::AcceptAsset(const std::function<void(const std::string&)>& callback)
    {
        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_PATH"))
            {
                const char* path = static_cast<const char*>(payload->Data);
                if (path && path[0] != '\0')
                    callback(path);
                ImGui::EndDragDropTarget();
                return true;
            }
            ImGui::EndDragDropTarget();
        }
        return false;
    }

    void UIUtils::SetTheme()
    {
        ImGuiStyle& style = ImGui::GetStyle();

        // Rounding — sharp, clinical
        style.WindowRounding    = 0.0f;
        style.ChildRounding     = 0.0f;
        style.FrameRounding     = 0.0f;
        style.PopupRounding     = 0.0f;
        style.ScrollbarRounding = 0.0f;
        style.GrabRounding      = 0.0f;
        style.TabRounding       = 0.0f;

        // Sizing
        style.WindowBorderSize  = 1.0f;
        style.FrameBorderSize   = 1.0f;
        style.PopupBorderSize   = 1.0f;
        style.FramePadding      = ImVec2(6, 4);
        style.ItemSpacing       = ImVec2(8, 4);
        style.IndentSpacing     = 14.0f;
        style.ScrollbarSize     = 10.0f;
        style.GrabMinSize       = 8.0f;

        ImVec4* c = style.Colors;

        // Pure monochromatic black and white
        c[ImGuiCol_WindowBg]             = ImVec4(0.95f, 0.95f, 0.95f, 1.00f);
        c[ImGuiCol_ChildBg]              = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
        c[ImGuiCol_PopupBg]              = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);

        // Borders — hard black
        c[ImGuiCol_Border]               = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
        c[ImGuiCol_BorderShadow]         = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);

        // Title bars — white (BeginEditorWindow overdraws these anyway)
        c[ImGuiCol_TitleBg]              = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
        c[ImGuiCol_TitleBgActive]        = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
        c[ImGuiCol_TitleBgCollapsed]     = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);

        // Frames — white fill, border provides visual weight
        c[ImGuiCol_FrameBg]              = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
        c[ImGuiCol_FrameBgHovered]       = ImVec4(0.85f, 0.85f, 0.85f, 1.00f);
        c[ImGuiCol_FrameBgActive]        = ImVec4(0.70f, 0.70f, 0.70f, 1.00f);

        // Scrollbar
        c[ImGuiCol_ScrollbarBg]          = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
        c[ImGuiCol_ScrollbarGrab]        = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
        c[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
        c[ImGuiCol_ScrollbarGrabActive]  = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);

        // Headers (TreeNode, CollapsingHeader, Selectable)
        c[ImGuiCol_Header]               = ImVec4(0.82f, 0.82f, 0.82f, 1.00f);
        c[ImGuiCol_HeaderHovered]        = ImVec4(0.70f, 0.70f, 0.70f, 1.00f);
        c[ImGuiCol_HeaderActive]         = ImVec4(0.55f, 0.55f, 0.55f, 1.00f);

        // Buttons — white fill, black border carries the weight
        c[ImGuiCol_Button]               = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
        c[ImGuiCol_ButtonHovered]        = ImVec4(0.85f, 0.85f, 0.85f, 1.00f);
        c[ImGuiCol_ButtonActive]         = ImVec4(0.65f, 0.65f, 0.65f, 1.00f);

        // Tabs
        c[ImGuiCol_Tab]                  = ImVec4(0.85f, 0.85f, 0.85f, 1.00f);
        c[ImGuiCol_TabHovered]           = ImVec4(0.70f, 0.70f, 0.70f, 1.00f);
        c[ImGuiCol_TabActive]            = ImVec4(0.95f, 0.95f, 0.95f, 1.00f); // matches WindowBg — feels "open"
        c[ImGuiCol_TabUnfocused]         = ImVec4(0.88f, 0.88f, 0.88f, 1.00f);
        c[ImGuiCol_TabUnfocusedActive]   = ImVec4(0.92f, 0.92f, 0.92f, 1.00f);

        // Docking
        c[ImGuiCol_DockingPreview]       = ImVec4(0.00f, 0.00f, 0.00f, 0.50f);
        c[ImGuiCol_DockingEmptyBg]       = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);

        // Selection / highlight — ink stamp
        c[ImGuiCol_CheckMark]            = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
        c[ImGuiCol_SliderGrab]           = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
        c[ImGuiCol_SliderGrabActive]     = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);

        // Text — pure black on white
        c[ImGuiCol_Text]                 = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
        c[ImGuiCol_TextDisabled]         = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
        c[ImGuiCol_TextSelectedBg]       = ImVec4(0.00f, 0.00f, 0.00f, 0.25f);

        // Separator — ruled line
        c[ImGuiCol_Separator]            = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
        c[ImGuiCol_SeparatorHovered]     = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
        c[ImGuiCol_SeparatorActive]      = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);

        // Resize grip
        c[ImGuiCol_ResizeGrip]           = ImVec4(0.00f, 0.00f, 0.00f, 0.25f);
        c[ImGuiCol_ResizeGripHovered]    = ImVec4(0.00f, 0.00f, 0.00f, 0.60f);
        c[ImGuiCol_ResizeGripActive]     = ImVec4(0.00f, 0.00f, 0.00f, 0.90f);

        // Menubar — white
        c[ImGuiCol_MenuBarBg]            = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);

        // Modal overlay
        c[ImGuiCol_ModalWindowDimBg]     = ImVec4(0.00f, 0.00f, 0.00f, 0.50f);

        // Nav highlight
        c[ImGuiCol_NavHighlight]         = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
        c[ImGuiCol_NavWindowingHighlight]= ImVec4(0.00f, 0.00f, 0.00f, 0.70f);
        c[ImGuiCol_NavWindowingDimBg]    = ImVec4(0.00f, 0.00f, 0.00f, 0.20f);
    }
}
