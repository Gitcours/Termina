#include "WorldHierarchyPanel.hpp"

#include "ImGui/imgui.h"
#include "Termina/Renderer/UIUtils.hpp"
#include <Termina/Core/Application.hpp>
#include <Termina/World/WorldSystem.hpp>
#include <Termina/World/World.hpp>
#include <Termina/World/Actor.hpp>

void WorldHierarchyPanel::DrawActorNode(Termina::Actor* actor)
{
    ImGuiTreeNodeFlags flags =
        ImGuiTreeNodeFlags_OpenOnArrow |
        ImGuiTreeNodeFlags_SpanAvailWidth;

    if (actor->GetChildren().empty())
        flags |= ImGuiTreeNodeFlags_Leaf;

    if (m_Context.SelectedActor == actor)
        flags |= ImGuiTreeNodeFlags_Selected;

    if (!actor->IsActive())
        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));

    bool open = ImGui::TreeNodeEx(
        (void*)(uintptr_t)actor->GetID(),
        flags, "%s", actor->GetName().c_str());

    if (!actor->IsActive())
        ImGui::PopStyleColor();

    if (ImGui::IsItemClicked())
        m_Context.SelectedActor = actor;

    if (ImGui::BeginPopupContextItem())
    {
        if (ImGui::MenuItem("Spawn Child"))
        {
            auto* child = actor->GetParentWorld()->SpawnActor();
            actor->AttachChild(child);
        }
        if (ImGui::MenuItem("Destroy"))
        {
            if (m_Context.SelectedActor == actor)
                m_Context.SelectedActor = nullptr;
            actor->GetParentWorld()->DestroyActor(actor);
            if (open) ImGui::TreePop();
            ImGui::EndPopup();
            return;
        }
        ImGui::EndPopup();
    }

    if (open)
    {
        for (auto* child : actor->GetChildren())
            DrawActorNode(child);
        ImGui::TreePop();
    }
}

void WorldHierarchyPanel::OnImGuiRender()
{
    Termina::UIUtils::BeginEditorWindow(m_Name.c_str(), &m_Open);

    auto* worldSystem = Termina::Application::GetSystem<Termina::WorldSystem>();
    Termina::World* world = worldSystem ? worldSystem->GetCurrentWorld() : nullptr;

    if (!world)
    {
        ImGui::TextDisabled("No world loaded");
        Termina::UIUtils::EndEditorWindow();
        return;
    }

    ImGui::TextDisabled("%s", world->GetName().c_str());
    ImGui::Separator();

    for (auto* root : world->GetRootActors())
        DrawActorNode(root);

    // Right-click on empty space to spawn a root actor.
    if (ImGui::BeginPopupContextWindow(
            "##hierarchy_ctx",
            ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
    {
        if (ImGui::MenuItem("Spawn Actor"))
            world->SpawnActor();
        ImGui::EndPopup();
    }

    Termina::UIUtils::EndEditorWindow();
}
