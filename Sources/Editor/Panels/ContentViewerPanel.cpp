#include "ContentViewerPanel.hpp"

#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"
#include "Termina/Renderer/UIUtils.hpp"

#include <Termina/Core/Application.hpp>
#include <Termina/Asset/AssetSystem.hpp>
#include <Termina/World/Actor.hpp>
#include <Termina/World/World.hpp>

#include <filesystem>
#include <fstream>
#include <vector>
#include <algorithm>
#include <cstring>

namespace fs = std::filesystem;

// ---------------------------------------------------------------------------
// Navigation helper
// ---------------------------------------------------------------------------

void ContentViewerPanel::Navigate(const fs::path& path)
{
    if (path == m_CurrentPath)
        return;

    m_BackHistory.push_back(m_CurrentPath);
    m_ForwardHistory.clear();
    m_CurrentPath = path;
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

std::string ContentViewerPanel::UniquePath(const fs::path& path)
{
    if (!fs::exists(path))
        return path.string();

    fs::path stem  = path.stem();
    fs::path ext   = path.extension();
    fs::path parent = path.parent_path();

    int n = 1;
    fs::path candidate;
    do {
        candidate = parent / (stem.string() + "_" + std::to_string(n) + ext.string());
        ++n;
    } while (fs::exists(candidate));

    return candidate.string();
}

const char* ContentViewerPanel::GetFileIcon(const fs::path& path) const
{
    std::string ext = path.extension().string();
    if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp")
        return "[IMG]";
    if (ext == ".hlsl" || ext == ".hlsli")
        return "[SHD]";
    if (ext == ".mp3" || ext == ".wav" || ext == ".ogg")
        return "[SND]";
    if (ext == ".ttf" || ext == ".otf")
        return "[FNT]";
    if (ext == ".trw")
        return "[WLD]";
    if (ext == ".mat")
        return "[MAT]";
    if (ext == ".gltf" || ext == ".glb")
        return "[MDL]";
    if (ext == ".trp")
        return "[PFB]";
    return "[FILE]";
}

// ---------------------------------------------------------------------------
// Asset creation
// ---------------------------------------------------------------------------

void ContentViewerPanel::CreateDefaultMaterial(const fs::path& dir)
{
    std::string dest = UniquePath(dir / "NewMaterial.mat");
    std::ofstream f(dest);
    f << R"({
  "albedo_texture": "",
  "normal_texture": "",
  "orm_texture": "",
  "emissive_texture": "",
  "alpha_test": false,
  "color": [1.0, 1.0, 1.0],
  "override_metallic": false,
  "override_roughness": false,
  "metallic_factor": 0.0,
  "roughness_factor": 0.5
})";
}

void ContentViewerPanel::CreateDefaultWorld(const fs::path& dir)
{
    std::string dest = UniquePath(dir / "NewWorld.trw");
    std::ofstream f(dest);
    // Minimal world: one Camera entity with Transform + Camera Component (primary)
    f << R"({"actors":[{"active":true,"components":[{"active":true,"data":{"position":[0.0,2.0,5.0],"rotation":[1.0,0.0,0.0,0.0],"scale":[1.0,1.0,1.0]},"type":"Transform"},{"active":true,"data":{"far":100.0,"fov":75.0,"near":0.10000000149011612,"primary":true},"type":"Camera Component"}],"id":"1","name":"Camera","parentId":null}],"modules":[],"name":"New World","version":1})";
}

// ---------------------------------------------------------------------------
// Context menus
// ---------------------------------------------------------------------------

void ContentViewerPanel::DrawContextMenuBackground()
{
    if (!ImGui::BeginPopupContextWindow("##bg_ctx", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
        return;

    if (ImGui::MenuItem("New Folder"))
    {
        m_ShowNewFolderModal = true;
        std::memset(m_ModalInputBuf, 0, sizeof(m_ModalInputBuf));
        std::strncpy(m_ModalInputBuf, "NewFolder", sizeof(m_ModalInputBuf) - 1);
    }

    ImGui::Separator();

    if (ImGui::MenuItem("New Material"))
        CreateDefaultMaterial(m_CurrentPath);

    if (ImGui::MenuItem("New World"))
        CreateDefaultWorld(m_CurrentPath);

    ImGui::EndPopup();
}

void ContentViewerPanel::DrawContextMenuEntry(const std::string& entryPath, bool /*isDir*/)
{
    std::string popupId = "##ctx_" + entryPath;
    if (!ImGui::BeginPopupContextItem(popupId.c_str()))
        return;

    if (ImGui::MenuItem("Rename"))
    {
        m_ShowRenameModal  = true;
        m_ModalTargetPath  = entryPath;
        std::memset(m_ModalInputBuf, 0, sizeof(m_ModalInputBuf));
        fs::path p(entryPath);
        std::strncpy(m_ModalInputBuf, p.filename().string().c_str(), sizeof(m_ModalInputBuf) - 1);
    }

    if (ImGui::MenuItem("Move To..."))
    {
        m_ShowMoveModal   = true;
        m_ModalTargetPath = entryPath;
        std::memset(m_ModalInputBuf, 0, sizeof(m_ModalInputBuf));
        // Pre-fill with the current parent dir as a starting point
        std::strncpy(m_ModalInputBuf, fs::path(entryPath).parent_path().string().c_str(),
                     sizeof(m_ModalInputBuf) - 1);
    }

    ImGui::Separator();

    if (ImGui::MenuItem("Delete"))
    {
        std::error_code ec;
        fs::remove_all(entryPath, ec);
        if (m_SelectedEntry == entryPath)
            m_SelectedEntry.clear();
    }

    ImGui::EndPopup();
}

// ---------------------------------------------------------------------------
// Modals
// ---------------------------------------------------------------------------

void ContentViewerPanel::DrawModals()
{
    // --- New Folder ---
    if (m_ShowNewFolderModal)
    {
        ImGui::OpenPopup("New Folder##modal");
        m_ShowNewFolderModal = false;
    }
    if (ImGui::BeginPopupModal("New Folder##modal", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Folder name:");
        ImGui::SetNextItemWidth(300.0f);
        bool confirm = ImGui::InputText("##foldername", m_ModalInputBuf, sizeof(m_ModalInputBuf),
                                        ImGuiInputTextFlags_EnterReturnsTrue);
        ImGui::Spacing();
        if (ImGui::Button("Create") || confirm)
        {
            fs::path newDir = m_CurrentPath / m_ModalInputBuf;
            std::error_code ec;
            fs::create_directory(newDir, ec);
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel"))
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }

    // --- Rename ---
    if (m_ShowRenameModal)
    {
        ImGui::OpenPopup("Rename##modal");
        m_ShowRenameModal = false;
    }
    if (ImGui::BeginPopupModal("Rename##modal", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("New name for:  %s", fs::path(m_ModalTargetPath).filename().string().c_str());
        ImGui::SetNextItemWidth(300.0f);
        bool confirm = ImGui::InputText("##rename", m_ModalInputBuf, sizeof(m_ModalInputBuf),
                                        ImGuiInputTextFlags_EnterReturnsTrue);
        ImGui::Spacing();
        if (ImGui::Button("Rename") || confirm)
        {
            fs::path src(m_ModalTargetPath);
            fs::path dst = src.parent_path() / m_ModalInputBuf;
            std::error_code ec;
            fs::rename(src, dst, ec);
            if (m_SelectedEntry == m_ModalTargetPath)
                m_SelectedEntry = dst.string();
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel"))
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }

    // --- Move To ---
    if (m_ShowMoveModal)
    {
        ImGui::OpenPopup("Move To##modal");
        m_ShowMoveModal = false;
    }
    if (ImGui::BeginPopupModal("Move To##modal", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Moving:  %s", fs::path(m_ModalTargetPath).filename().string().c_str());
        ImGui::Text("Destination directory:");
        ImGui::SetNextItemWidth(400.0f);
        bool confirm = ImGui::InputText("##moveto", m_ModalInputBuf, sizeof(m_ModalInputBuf),
                                        ImGuiInputTextFlags_EnterReturnsTrue);
        ImGui::Spacing();
        if (ImGui::Button("Move") || confirm)
        {
            fs::path src(m_ModalTargetPath);
            fs::path destDir(m_ModalInputBuf);
            if (fs::is_directory(destDir))
            {
                fs::path dst = destDir / src.filename();
                std::error_code ec;
                fs::rename(src, dst, ec);
                if (m_SelectedEntry == m_ModalTargetPath)
                    m_SelectedEntry = dst.string();
            }
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel"))
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
}

// ---------------------------------------------------------------------------
// Main render
// ---------------------------------------------------------------------------

void ContentViewerPanel::OnImGuiRender()
{
    m_Context.ContentViewer = this;

    Termina::UIUtils::BeginEditorWindow(m_Name.c_str(), &m_Open);

    // Navigation buttons (Back/Forward)
    {
        ImGui::BeginDisabled(m_BackHistory.empty());
        if (ImGui::Button("<"))
        {
            m_ForwardHistory.push_back(m_CurrentPath);
            m_CurrentPath = m_BackHistory.back();
            m_BackHistory.pop_back();
        }
        ImGui::EndDisabled();

        ImGui::SameLine();

        ImGui::BeginDisabled(m_ForwardHistory.empty());
        if (ImGui::Button(">"))
        {
            m_BackHistory.push_back(m_CurrentPath);
            m_CurrentPath = m_ForwardHistory.back();
            m_ForwardHistory.pop_back();
        }
        ImGui::EndDisabled();

        ImGui::SameLine();
        ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
        ImGui::SameLine();
    }

    // Breadcrumb navigation
    {
        fs::path root("Assets");
        fs::path rel = fs::relative(m_CurrentPath, root.parent_path());
        std::vector<fs::path> parts;
        for (const auto& part : rel)
            parts.push_back(part);

        fs::path accumulated = root.parent_path();
        for (size_t i = 0; i < parts.size(); ++i)
        {
            accumulated /= parts[i];
            if (i > 0)
                ImGui::SameLine();
            ImGui::PushID((int)i);
            if (ImGui::SmallButton(parts[i].string().c_str()))
                Navigate(accumulated);
            ImGui::PopID();
            if (i + 1 < parts.size())
            {
                ImGui::SameLine();
                ImGui::TextDisabled(">");
            }
        }
    }

    ImGui::Separator();

    if (!fs::exists(m_CurrentPath) || !fs::is_directory(m_CurrentPath))
    {
        ImGui::TextDisabled("(directory not found)");
        Termina::UIUtils::EndEditorWindow();
        return;
    }

    // Collect entries: folders first, then files
    std::vector<fs::directory_entry> folders, files;
    for (const auto& entry : fs::directory_iterator(m_CurrentPath))
    {
        if (entry.is_directory())
            folders.push_back(entry);
        else
            files.push_back(entry);
    }
    std::sort(folders.begin(), folders.end(), [](const auto& a, const auto& b) {
        return a.path().filename() < b.path().filename();
    });
    std::sort(files.begin(), files.end(), [](const auto& a, const auto& b) {
        return a.path().filename() < b.path().filename();
    });

    // Render folders
    for (const auto& entry : folders)
    {
        std::string name  = entry.path().filename().string();
        std::string label = "[DIR]  " + name;
        bool selected     = (m_SelectedEntry == entry.path().string());

        if (ImGui::Selectable(label.c_str(), selected, ImGuiSelectableFlags_AllowDoubleClick))
        {
            m_SelectedEntry = entry.path().string();
            if (ImGui::IsMouseDoubleClicked(0))
                Navigate(entry.path());
        }

        Termina::UIUtils::AcceptActor([this, entry](Termina::Actor* dragged) {
            std::string filename = std::string(dragged->GetName()) + ".trp";
            fs::path finalPath = UniquePath(entry.path() / filename);
            dragged->GetParentWorld()->SaveActorToJSON(dragged, finalPath.generic_string());
        });

        DrawContextMenuEntry(entry.path().string(), true);
    }

    // Render files
    for (const auto& entry : files)
    {
        std::string name  = entry.path().filename().string();
        const char* icon  = GetFileIcon(entry.path());
        std::string label = std::string(icon) + "  " + name;
        bool selected     = (m_SelectedEntry == entry.path().string());

        if (ImGui::Selectable(label.c_str(), selected))
        {
            m_SelectedEntry = entry.path().string();

            // Load material into inspector when a .mat file is selected
            if (entry.path().extension() == ".mat")
            {
                auto* assets = Termina::Application::GetSystem<Termina::AssetSystem>();
                m_InspectedMaterial = assets->Load<Termina::MaterialAsset>(entry.path().string());
                m_Context.ItemToInspect = m_InspectedMaterial.Get();
            }
            else
            {
                m_InspectedMaterial = {};
                m_Context.ItemToInspect = nullptr;
            }
        }

        DrawContextMenuEntry(entry.path().string(), false);

        // Drag source for asset picking
        Termina::UIUtils::AssetPickerSource(entry.path().string());
    }

    // Drop target on empty space in current folder
    ImVec2 remaining = ImGui::GetContentRegionAvail();
    if (remaining.y > 0.0f)
    {
        ImGui::InvisibleButton("##content_drop_target", remaining);
        Termina::UIUtils::AcceptActor([this](Termina::Actor* dragged) {
            std::string filename = std::string(dragged->GetName()) + ".trp";
            fs::path finalPath = UniquePath(m_CurrentPath / filename);
            dragged->GetParentWorld()->SaveActorToJSON(dragged, finalPath.generic_string());
        });
    }

    // Right-click on empty space
    DrawContextMenuBackground();

    // Modals (must be inside the same ImGui window scope)
    DrawModals();

    Termina::UIUtils::EndEditorWindow();
}
