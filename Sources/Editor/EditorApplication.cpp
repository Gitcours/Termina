#include "EditorApplication.hpp"

#include "Panels/ViewportPanel.hpp"
#include "Panels/WorldHierarchyPanel.hpp"
#include "Panels/InspectorPanel.hpp"
#include "Panels/ContentViewerPanel.hpp"

#include <Termina/Renderer/Renderer.hpp>
#include <Termina/Renderer/UIUtils.hpp>
#include <Termina/Shader/ShaderManager.hpp>
#include <Termina/Core/Logger.hpp>

#include "ImGui/imgui.h"
#include "Termina/Asset/AssetSystem.hpp"
#include "Termina/Audio/AudioSystem.hpp"
#include "Termina/Input/InputSystem.hpp"
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

    RegisterPanel<ViewportPanel>();
    RegisterPanel<WorldHierarchyPanel>();
    RegisterPanel<InspectorPanel>();
    RegisterPanel<ContentViewerPanel>();

    Termina::ComponentRegistry::Get().Report();

    Termina::World* world = GetSystem<Termina::WorldSystem>()->GetCurrentWorld();
    world->LoadFromFile("Assets/Worlds/Cubes.trw");
}

EditorApplication::~EditorApplication()
{
}

void EditorApplication::OnUpdate(float dt)
{
    auto* renderer = GetSystem<Termina::RendererSystem>();
    float w = static_cast<float>(m_Window->GetWidth());
    float h = static_cast<float>(m_Window->GetHeight());
    if (!ImGui::GetIO().WantCaptureKeyboard)
        m_Camera.Update(dt, w, h);
    if (!GetSystem<Termina::WorldSystem>()->IsPlaying())
        renderer->SetCurrentCamera(m_Camera);

    RenderDockspace();

    for (auto& panel : m_Panels) {
        if (panel->IsOpen())
            panel->OnImGuiRender();
    }
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

        Termina::UIUtils::EndMenuBar();
    }
    Termina::UIUtils::PopStylized();

    ImGuiID dockspaceId = ImGui::GetID("DockspaceRoot");
    ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);

    ImGui::End();
}
