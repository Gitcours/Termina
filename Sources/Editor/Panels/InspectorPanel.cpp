#include "InspectorPanel.hpp"

#include "ImGui/imgui.h"
#include "Termina/Renderer/UIUtils.hpp"
#include <Termina/World/Actor.hpp>
#include <Termina/World/ComponentRegistry.hpp>

#include <cstring>
#include <typeindex>
#include <unordered_set>

void InspectorPanel::OnImGuiRender()
{
    Termina::UIUtils::BeginEditorWindow(m_Name.c_str(), &m_Open);

    Termina::Actor* actor = m_Context.SelectedActor;
    if (!actor)
    {
        ImGui::TextDisabled("No actor selected");
        Termina::UIUtils::EndEditorWindow();
        return;
    }

    // Name
    char nameBuf[256];
    strncpy(nameBuf, actor->GetName().c_str(), sizeof(nameBuf) - 1);
    nameBuf[sizeof(nameBuf) - 1] = '\0';
    ImGui::SetNextItemWidth(-1.0f);
    if (ImGui::InputText("##name", nameBuf, sizeof(nameBuf)))
        actor->SetName(nameBuf);

    // Active flag
    bool active = actor->IsActive();
    if (ImGui::Checkbox("Active", &active))
        actor->SetActive(active);

    ImGui::Separator();

    // Components — each gets a collapsing header and calls its own Inspect()
    for (auto* comp : actor->GetAllComponents())
    {
        std::string compName =
            Termina::ComponentRegistry::Get().GetNameForType(typeid(*comp));
        if (compName.empty())
            compName = "Unknown Component";

        ImGui::PushID(comp);
        Termina::UIUtils::PushStylized();
        if (ImGui::TreeNodeEx(compName.c_str(), ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed)) {
            Termina::UIUtils::PopStylized();
            comp->Inspect();
            ImGui::TreePop();
        } else
            Termina::UIUtils::PopStylized();
        ImGui::PopID();
    }

    ImGui::Separator();

    // Collect types already on the actor
    std::unordered_set<std::type_index> existing;
    for (auto* c : actor->GetAllComponents())
        existing.insert(typeid(*c));

    Termina::UIUtils::PushStylized();
    if (Termina::UIUtils::Button("Add Component"))
        ImGui::OpenPopup("##AddComponent");
    Termina::UIUtils::PopStylized();

    if (ImGui::BeginPopup("##AddComponent")) {
        std::type_index selectedType = typeid(void);
        Termina::ComponentRegistry::Get().ForEach([&](const Termina::ComponentRegistry::Entry& entry) {
            if (existing.count(entry.Type))
                return true;
            if (ImGui::MenuItem(entry.Name.c_str()))
                selectedType = entry.Type;
            return true;
        });
        ImGui::EndPopup();

        if (selectedType != typeid(void)) {
            auto* comp = Termina::ComponentRegistry::Get().CreateByType(selectedType, actor);
            if (comp)
                actor->AddComponentRaw(comp);
        }
    }

    Termina::UIUtils::EndEditorWindow();
}
