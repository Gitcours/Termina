#include "EditorApplication.hpp"

#include "Panels/ViewportPanel.hpp"
#include "Panels/WorldHierarchyPanel.hpp"
#include "Panels/InspectorPanel.hpp"
#include "Panels/ContentViewerPanel.hpp"

#include <Termina/Renderer/Renderer.hpp>
#include <Termina/Renderer/UIUtils.hpp>
#include <Termina/Renderer/Passes/DebugPass.hpp>
#include <Termina/Shader/ShaderManager.hpp>
#include <Termina/Core/Logger.hpp>

#include "ImGui/imgui.h"
#include "Termina/Asset/AssetSystem.hpp"
#include "Termina/Audio/AudioSystem.hpp"
#include "Termina/Core/SystemManager.hpp"
#include "Termina/Input/InputSystem.hpp"
#include "Termina/Physics/PhysicsSystem.hpp"
#include "Termina/Platform/FileDialog.hpp"
#include "Termina/Platform/LaunchProcess.hpp"
#include "Termina/Scripting/ScriptSystem.hpp"
#include "Termina/World/ComponentRegistry.hpp"
#include "Termina/World/World.hpp"
#include "Termina/World/WorldSystem.hpp"

#include <Termina/Core/Version.hpp>

#include <filesystem>

EditorApplication::EditorApplication(const std::string& projectPath)
    : Application("Editor")
{
    m_SystemManager.AddSystem<Termina::InputSystem>(m_Window->GetHandle());
    m_SystemManager.AddSystem<Termina::WorldSystem>();
    m_SystemManager.AddSystem<Termina::RendererSystem>(m_Window);
    m_SystemManager.AddSystem<Termina::ShaderManager>();
    m_SystemManager.AddSystem<Termina::AudioSystem>();
    m_SystemManager.AddSystem<Termina::ScriptSystem>();
    m_SystemManager.AddSystem<Termina::AssetSystem>();
    m_SystemManager.AddSystem<Termina::PhysicsSystem>();

    RegisterPanel<ViewportPanel>();
    RegisterPanel<WorldHierarchyPanel>();
    RegisterPanel<InspectorPanel>();
    RegisterPanel<ContentViewerPanel>();

    Termina::ComponentRegistry::Get().Report();

    if (!projectPath.empty())
    {
        m_Context.CurrentProject.LoadProject(projectPath);
        std::filesystem::current_path(m_Context.CurrentProject.Path);
    }
}

EditorApplication::~EditorApplication()
{
    m_Context.ItemToInspect = nullptr;
    m_Context.SelectedActors.clear();
    m_Context.ContentViewer = nullptr;
    m_Panels.clear();
}

void EditorApplication::OnUpdate(float dt)
{
    m_Context.LastDeltaTime = dt;

    auto* renderer = GetSystem<Termina::RendererSystem>();
    float w = m_Context.ViewportWidth > 0.0f ? m_Context.ViewportWidth : static_cast<float>(m_Window->GetWidth());
    float h = m_Context.ViewportHeight > 0.0f ? m_Context.ViewportHeight : static_cast<float>(m_Window->GetHeight());
    if (!ImGui::GetIO().WantCaptureKeyboard && !GetSystem<Termina::WorldSystem>()->IsPlaying())
        m_Camera.Update(dt, w, h);
    if (!GetSystem<Termina::WorldSystem>()->IsPlaying()) {
        renderer->SetCurrentCamera(m_Camera);
        Termina::DebugPass::DrawGrid(glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f), 250.0f, 250);
    }

    RenderDockspace();

    for (auto& panel : m_Panels) {
        if (panel->IsOpen())
            panel->OnImGuiRender();
    }

    if (m_DebugWindows.SystemManager) m_SystemManager.ShowDebugWindow(&m_DebugWindows.SystemManager);
    if (m_DebugWindows.Input)         Termina::InputSystem::ShowDebugWindow(&m_DebugWindows.Input);
    if (m_DebugWindows.Renderer)      GetSystem<Termina::RendererSystem>()->ShowDebugWindow(&m_DebugWindows.Renderer);
    if (m_DebugWindows.Scripts)       GetSystem<Termina::ScriptSystem>()->ShowDebugWindow(&m_DebugWindows.Scripts);
    if (m_DebugWindows.Shaders)       GetSystem<Termina::ShaderManager>()->ShowDebugWindow(&m_DebugWindows.Shaders);
    if (m_DebugWindows.Assets)        GetSystem<Termina::AssetSystem>()->ShowDebugWindow(&m_DebugWindows.Assets);
    if (m_DebugWindows.Physics)       GetSystem<Termina::PhysicsSystem>()->ShowDebugWindow(&m_DebugWindows.Physics);
    if (m_DebugWindows.Log)           Termina::Logger::ShowLogWindow(&m_DebugWindows.Log);
}

void EditorApplication::OnPostRender(float dt)
{
    Termina::AssetSystem* assetSystem = GetSystem<Termina::AssetSystem>();
    assetSystem->Clean(0);
}

bool EditorApplication::SaveWorld(bool forceDialog)
{
    auto* worldSystem = GetSystem<Termina::WorldSystem>();
    auto* world = worldSystem->GetCurrentWorld();

    std::string path = world->GetCurrentPath();
    if (forceDialog || path.empty())
    {
        path = Termina::FileDialog::SaveFile();
        if (path.empty())
            return false;
    }

    return worldSystem->SaveWorld(path);
}

void EditorApplication::OpenWorld()
{
    std::string path = Termina::FileDialog::OpenFile();
    if (path.empty())
        return;

    // Save the current world before discarding it.
    SaveWorld();

    auto* worldSystem = GetSystem<Termina::WorldSystem>();
    m_Context.ItemToInspect = nullptr;
    m_Context.SelectedActors.clear();
    if (worldSystem->LoadWorld(path))
    {
        // Both old world (now unloaded) and new world are settled;
        // clean up assets that were only held by the old scene.
        GetSystem<Termina::AssetSystem>()->Clean(0);
    }
}

void EditorApplication::NewWorld()
{
    // Save the current world before replacing it.
    SaveWorld();

    m_Context.ItemToInspect = nullptr;
    m_Context.SelectedActors.clear();
    auto* worldSystem = GetSystem<Termina::WorldSystem>();
    worldSystem->NewWorld();

    // Clean up assets that were only held by the old scene.
    GetSystem<Termina::AssetSystem>()->Clean(1);
}

void EditorApplication::RenderDockspace()
{
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGuiWindowFlags windowFlags =
        ImGuiWindowFlags_NoDocking |
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoNavFocus |
        ImGuiWindowFlags_MenuBar;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("##DockspaceRoot", nullptr, windowFlags);
    ImGui::PopStyleVar(3);

    Termina::UIUtils::PushStylized();
    if (Termina::UIUtils::BeginMenuBar())
    {
        if (Termina::UIUtils::BeginMenu("Termina"))
        {
            if (Termina::UIUtils::MenuItem("Quit"))
                m_Running = false;
            ImGui::Separator();
            if (Termina::UIUtils::MenuItem("About"))
                m_ShowAbout = true;
            Termina::UIUtils::EndMenu();
        }

        if (Termina::UIUtils::BeginMenu("World"))
        {
            if (Termina::UIUtils::MenuItem("New", "Ctrl+N"))
                NewWorld();
            if (Termina::UIUtils::MenuItem("Open...", "Ctrl+O"))
                OpenWorld();
            ImGui::Separator();
            if (Termina::UIUtils::MenuItem("Save", "Ctrl+S"))
                SaveWorld();
            if (Termina::UIUtils::MenuItem("Save As..."))
                SaveWorld(true);
            Termina::UIUtils::EndMenu();
        }

        if (Termina::UIUtils::BeginMenu("Layout"))
        {
            if (Termina::UIUtils::BeginMenu("Theme"))
            {
                bool isDark = Termina::UIUtils::IsDarkTheme();
                if (Termina::UIUtils::MenuItem("Termina (Light)", nullptr, !isDark))
                    Termina::UIUtils::SetDarkTheme(false);
                if (Termina::UIUtils::MenuItem("PRHVL BOP (Dark)", nullptr, isDark))
                    Termina::UIUtils::SetDarkTheme(true);
                Termina::UIUtils::EndMenu();
            }
            ImGui::Separator();
            for (auto& panel : m_Panels)
            {
                bool open = panel->IsOpen();
                if (Termina::UIUtils::MenuItem(panel->GetName().c_str(), nullptr, open))
                    panel->SetOpen(!open);
            }
            Termina::UIUtils::EndMenu();
        }

        if (Termina::UIUtils::BeginMenu("Scripting"))
        {
            if (Termina::UIUtils::MenuItem("Compile", "F5"))
                GetSystem<Termina::ScriptSystem>()->Compile();
            if (Termina::UIUtils::MenuItem("Recompile", "Ctrl+F5"))
                GetSystem<Termina::ScriptSystem>()->Recompile();
            Termina::UIUtils::EndMenu();
        }

        if (Termina::UIUtils::BeginMenu("Debug"))
        {
            if (Termina::UIUtils::MenuItem("System Manager", nullptr, m_DebugWindows.SystemManager)) m_DebugWindows.SystemManager = !m_DebugWindows.SystemManager;
            if (Termina::UIUtils::MenuItem("Input",          nullptr, m_DebugWindows.Input))         m_DebugWindows.Input         = !m_DebugWindows.Input;
            if (Termina::UIUtils::MenuItem("Renderer",       nullptr, m_DebugWindows.Renderer))      m_DebugWindows.Renderer      = !m_DebugWindows.Renderer;
            if (Termina::UIUtils::MenuItem("Scripts",        nullptr, m_DebugWindows.Scripts))       m_DebugWindows.Scripts       = !m_DebugWindows.Scripts;
            if (Termina::UIUtils::MenuItem("Shaders",        nullptr, m_DebugWindows.Shaders))       m_DebugWindows.Shaders       = !m_DebugWindows.Shaders;
            if (Termina::UIUtils::MenuItem("Assets",         nullptr, m_DebugWindows.Assets))        m_DebugWindows.Assets        = !m_DebugWindows.Assets;
            if (Termina::UIUtils::MenuItem("Physics",        nullptr, m_DebugWindows.Physics))       m_DebugWindows.Physics       = !m_DebugWindows.Physics;
            if (Termina::UIUtils::MenuItem("Log",            nullptr, m_DebugWindows.Log))           m_DebugWindows.Log           = !m_DebugWindows.Log;
            Termina::UIUtils::EndMenu();
        }

        Termina::UIUtils::EndMenuBar();
    }
    Termina::UIUtils::PopStylized();

    // About modal
    if (m_ShowAbout) ImGui::OpenPopup("About Termina");
    ImGui::SetNextWindowSize(ImVec2(400, 160), ImGuiCond_Always);
    if (ImGui::BeginPopupModal("About Termina", nullptr, ImGuiWindowFlags_NoResize))
    {
        ImGui::Text("Termina Engine");
        ImGui::Separator();
        ImGui::Text("Version:    %s", Termina::TERMINA_VERSION);
        ImGui::Text("Author:     %s", Termina::TERMINA_AUTHOR);
        ImGui::Text("Build date: %s", Termina::TERMINA_BUILD_DATE);
        ImGui::Spacing();
        if (ImGui::Button("Close", ImVec2(120, 0)))
        {
            m_ShowAbout = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    ImGuiID dockspaceId = ImGui::GetID("DockspaceRoot");
    ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);

    ImGui::End();
}
