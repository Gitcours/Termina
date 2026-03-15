#include "WorldHierarchyPanel.hpp"

#include "ImGui/imgui.h"
#include "Termina/Renderer/UIUtils.hpp"
#include "ContentViewerPanel.hpp"
#include <Termina/Core/Application.hpp>
#include <Termina/World/WorldSystem.hpp>
#include <Termina/World/World.hpp>
#include <Termina/World/Actor.hpp>
#include <Termina/World/Components/Transform.hpp>

#include <GLM/glm.hpp>
#include <algorithm>

void WorldHierarchyPanel::DrawActorNode(Termina::Actor* actor)
{
    ImGuiTreeNodeFlags flags =
        ImGuiTreeNodeFlags_OpenOnArrow |
        ImGuiTreeNodeFlags_SpanAvailWidth;

    if (actor->GetChildren().empty())
        flags |= ImGuiTreeNodeFlags_Leaf;

    bool isSelected = (m_Context.ItemToInspect == actor) ||
        (std::find(m_Context.SelectedActors.begin(), m_Context.SelectedActors.end(), actor)
         != m_Context.SelectedActors.end());
    if (isSelected)
        flags |= ImGuiTreeNodeFlags_Selected;

    if (!actor->IsActive())
        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));

    bool open = ImGui::TreeNodeEx(
        (void*)(uintptr_t)actor->GetID(),
        flags, "%s", actor->GetName().c_str());

    if (!actor->IsActive())
        ImGui::PopStyleColor();

    // Drag source
    Termina::UIUtils::ActorPickerSource(actor);

    // Drop target: reparent dragged actor as child of this one, or spawn prefab
    Termina::UIUtils::AcceptActor([actor](Termina::Actor* dragged) {
        if (dragged != actor && !actor->IsDescendantOf(dragged))
            actor->AttachChild(dragged);
    });

    Termina::UIUtils::AcceptAsset([actor](const std::string& path) {
        if (path.find(".trp") != std::string::npos)
        {
            auto* spawned = actor->GetParentWorld()->SpawnActorFromJSON(path);
            if (spawned)
                actor->AttachChild(spawned);
        }
    });

    if (ImGui::IsItemClicked())
    {
        if (ImGui::GetIO().KeyCtrl)
        {
            auto it = std::find(m_Context.SelectedActors.begin(), m_Context.SelectedActors.end(), actor);
            if (it != m_Context.SelectedActors.end())
            {
                m_Context.SelectedActors.erase(it);
                if (m_Context.ItemToInspect == actor)
                    m_Context.ItemToInspect = m_Context.SelectedActors.empty() ? nullptr : m_Context.SelectedActors.back();
            }
            else
            {
                m_Context.SelectedActors.push_back(actor);
                m_Context.ItemToInspect = actor;
            }
        }
        else
        {
            m_Context.SelectedActors.clear();
            m_Context.SelectedActors.push_back(actor);
            m_Context.ItemToInspect = actor;
        }
    }

    bool shouldDestroy = false;
    bool shouldDestroySelection = false;
    if (ImGui::BeginPopupContextItem())
    {
        if (ImGui::MenuItem("Spawn Child"))
        {
            auto* child = actor->GetParentWorld()->SpawnActor();
            actor->AttachChild(child);
        }
        if (ImGui::MenuItem("Destroy"))
        {
            bool inSelection = std::find(m_Context.SelectedActors.begin(), m_Context.SelectedActors.end(), actor)
                               != m_Context.SelectedActors.end();
            if (inSelection && m_Context.SelectedActors.size() > 1)
                shouldDestroySelection = true;
            else
            {
                if (m_Context.ItemToInspect == actor)
                    m_Context.ItemToInspect = nullptr;
                m_Context.SelectedActors.erase(
                    std::remove(m_Context.SelectedActors.begin(), m_Context.SelectedActors.end(), actor),
                    m_Context.SelectedActors.end());
                shouldDestroy = true;
            }
        }
        ImGui::EndPopup();
    }

    // Destroy and TreePop must happen outside the popup's window context.
    if (shouldDestroy)
    {
        actor->GetParentWorld()->DestroyActor(actor);
        if (open) ImGui::TreePop();
        return;
    }

    if (shouldDestroySelection)
    {
        auto toDestroy = m_Context.SelectedActors;
        m_Context.SelectedActors.clear();
        m_Context.ItemToInspect = nullptr;
        auto* world = actor->GetParentWorld();
        for (auto* a : toDestroy)
            world->DestroyActor(a);
        if (open) ImGui::TreePop();
        return;
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

    ImGui::SameLine(ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize("Create").x - ImGui::GetStyle().FramePadding.x * 2.0f);
    Termina::UIUtils::PushStylized();
    if (ImGui::Button("Create"))
        ImGui::OpenPopup("##create_actor");
    Termina::UIUtils::PopStylized();

    if (ImGui::BeginPopup("##create_actor"))
    {
        if (ImGui::MenuItem("Empty"))
            world->SpawnActor();
        ImGui::EndPopup();
    }

    ImGui::Separator();

    // Keyboard shortcuts for clipboard and duplication
    if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows))
    {
        ImGuiIO& io = ImGui::GetIO();
        const bool ctrl = io.KeyCtrl;

        // Ctrl+C: Copy
        if (ctrl && ImGui::IsKeyPressed(ImGuiKey_C, false))
        {
            m_Context.Clipboard.clear();
            m_Context.ClipboardIsCut = false;
            for (auto* a : m_Context.SelectedActors)
                m_Context.Clipboard.push_back(a->GetID());
        }
        // Ctrl+X: Cut
        if (ctrl && ImGui::IsKeyPressed(ImGuiKey_X, false))
        {
            m_Context.Clipboard.clear();
            m_Context.ClipboardIsCut = true;
            for (auto* a : m_Context.SelectedActors)
                m_Context.Clipboard.push_back(a->GetID());
        }
        // Ctrl+V: Paste
        if (ctrl && ImGui::IsKeyPressed(ImGuiKey_V, false) && !m_Context.Clipboard.empty())
        {
            std::vector<Termina::Actor*> originals;
            for (uint64 id : m_Context.Clipboard)
                if (auto* a = world->GetActorById(id)) originals.push_back(a);

            m_Context.SelectedActors.clear();
            m_Context.ItemToInspect = nullptr;
            const glm::vec3 offset(0.5f, 0.0f, 0.5f);

            for (auto* src : originals)
            {
                auto* copy = world->SpawnActorFrom(src);
                if (copy)
                {
                    auto& t = copy->GetComponent<Termina::Transform>();
                    t.SetPosition(t.GetPosition() + offset);
                    m_Context.SelectedActors.push_back(copy);
                    m_Context.ItemToInspect = copy;
                }
            }
            if (m_Context.ClipboardIsCut)
            {
                for (auto* src : originals) world->DestroyActor(src);
                m_Context.Clipboard.clear();
                m_Context.ClipboardIsCut = false;
            }
        }
        // Ctrl+D: Duplicate
        if (ctrl && ImGui::IsKeyPressed(ImGuiKey_D, false) && !m_Context.SelectedActors.empty())
        {
            auto sources = m_Context.SelectedActors;
            m_Context.SelectedActors.clear();
            m_Context.ItemToInspect = nullptr;
            const glm::vec3 offset(0.5f, 0.0f, 0.5f);
            for (auto* src : sources)
            {
                auto* copy = world->SpawnActorFrom(src);
                if (copy)
                {
                    auto& t = copy->GetComponent<Termina::Transform>();
                    t.SetPosition(t.GetPosition() + offset);
                    m_Context.SelectedActors.push_back(copy);
                    m_Context.ItemToInspect = copy;
                }
            }
        }
    }

    for (auto* root : world->GetRootActors())
        DrawActorNode(root);

    // Drop target on empty space: demote dragged actor to root level or spawn prefab.
    ImVec2 remaining = ImGui::GetContentRegionAvail();
    if (remaining.y > 0.0f)
    {
        ImGui::InvisibleButton("##root_drop", remaining);
        
        Termina::UIUtils::AcceptActor([](Termina::Actor* dragged) {
            dragged->DetachFromParent();
        });

        Termina::UIUtils::AcceptAsset([world](const std::string& path) {
            if (path.find(".trp") != std::string::npos)
            {
                world->SpawnActorFromJSON(path);
            }
        });
    }



    Termina::UIUtils::EndEditorWindow();
}
