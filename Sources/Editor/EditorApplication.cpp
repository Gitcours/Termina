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

EditorApplication::EditorApplication()
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
}

EditorApplication::~EditorApplication()
{
    m_Context.ItemToInspect = nullptr;
    m_Context.ContentViewer = nullptr;
    m_Panels.clear();
}

void EditorApplication::OnUpdate(float dt)
{
    auto* renderer = GetSystem<Termina::RendererSystem>();
    float w = static_cast<float>(m_Window->GetWidth());
    float h = static_cast<float>(m_Window->GetHeight());
    if (!ImGui::GetIO().WantCaptureKeyboard && !GetSystem<Termina::WorldSystem>()->IsPlaying())
        m_Camera.Update(dt, w, h);
    if (!GetSystem<Termina::WorldSystem>()->IsPlaying())
        renderer->SetCurrentCamera(m_Camera);

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
            Termina::UIUtils::EndMenu();
        }

        Termina::UIUtils::EndMenuBar();
    }
    Termina::UIUtils::PopStylized();

    ImGuiID dockspaceId = ImGui::GetID("DockspaceRoot");
    ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);

    ImGui::End();
}
