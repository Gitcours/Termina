#include "Actor.hpp"
#include "Component.hpp"
#include "ComponentRegistry.hpp"
#include "Components/Transform.hpp"

#include "ImGui/imgui.h"
#include <Termina/Renderer/UIUtils.hpp>

#include <algorithm>
#include <cctype>
#include <typeindex>
#include <unordered_set>

namespace Termina {
    Actor::Actor(World* world, const std::string& name)
        : m_ParentWorld(world), m_Name(name)
    {
    }

    Actor::~Actor()
    {
        if (m_Parent) m_Parent->DetachChild(this);
        for (Actor* child : m_Children) child->m_Parent = nullptr;
        m_Children.clear();
        for (auto& component : m_Components) delete component;
        m_Components.clear();
        m_ComponentMap.clear();

        IDGenerator::Get().Release(m_ID);
    }

    void Actor::OnInit()
    {
        m_Initialized = true;
        for (Component* component : m_Components) component->OnInit();
    }

    void Actor::OnShutdown()
    {
        for (Component* component : m_Components) component->OnShutdown();
    }

    void Actor::OnPlay()
    {
        m_InPlayMode = true;
        for (Component* component : m_Components) component->OnPlay();
    }

    void Actor::OnStop()
    {
        for (Component* component : m_Components) component->OnStop();
        m_InPlayMode = false;
    }

    void Actor::OnPreUpdate(float deltaTime)
    {
        for (Component* component : m_Components)
            if (component->IsActive() && (m_InPlayMode || Any(component->GetUpdateFlags(), UpdateFlags::UpdateDuringEditor)))
                component->OnPreUpdate(deltaTime);
    }

    void Actor::OnUpdate(float deltaTime)
    {
        for (Component* component : m_Components)
            if (component->IsActive() && (m_InPlayMode || Any(component->GetUpdateFlags(), UpdateFlags::UpdateDuringEditor)))
                component->OnUpdate(deltaTime);
    }

    void Actor::OnPostUpdate(float deltaTime)
    {
        for (Component* component : m_Components)
            if (component->IsActive() && (m_InPlayMode || Any(component->GetUpdateFlags(), UpdateFlags::UpdateDuringEditor)))
                component->OnPostUpdate(deltaTime);
    }

    void Actor::OnPrePhysics(float deltaTime)
    {
        for (Component* component : m_Components)
            if (component->IsActive() && (m_InPlayMode || Any(component->GetUpdateFlags(), UpdateFlags::PhysicsUpdateDuringEditor)))
                component->OnPrePhysics(deltaTime);
    }

    void Actor::OnPhysics(float deltaTime)
    {
        for (Component* component : m_Components)
            if (component->IsActive() && (m_InPlayMode || Any(component->GetUpdateFlags(), UpdateFlags::PhysicsUpdateDuringEditor)))
                component->OnPhysics(deltaTime);
    }

    void Actor::OnPostPhysics(float deltaTime)
    {
        for (Component* component : m_Components)
            if (component->IsActive() && (m_InPlayMode || Any(component->GetUpdateFlags(), UpdateFlags::PhysicsUpdateDuringEditor)))
                component->OnPostPhysics(deltaTime);
    }

    void Actor::OnPreRender(float deltaTime)
    {
        for (Component* component : m_Components)
            if (component->IsActive() && (m_InPlayMode || Any(component->GetUpdateFlags(), UpdateFlags::RenderUpdateDuringEditor)))
                component->OnPreRender(deltaTime);
    }

    void Actor::OnRender(float deltaTime)
    {
        for (Component* component : m_Components)
            if (component->IsActive() && (m_InPlayMode || Any(component->GetUpdateFlags(), UpdateFlags::RenderUpdateDuringEditor)))
                component->OnRender(deltaTime);
    }

    void Actor::OnPostRender(float deltaTime)
    {
        for (Component* component : m_Components)
            if (component->IsActive() && (m_InPlayMode || Any(component->GetUpdateFlags(), UpdateFlags::RenderUpdateDuringEditor)))
                component->OnPostRender(deltaTime);
    }

    void Actor::OnAttach(Actor* newParent)
    {
        for (Component* component : m_Components) {
            component->OnAttach(newParent);
        }
    }

    void Actor::OnDetach(Actor* oldParent)
    {
        for (Component* component : m_Components) {
            component->OnDetach(oldParent);
        }
    }

    void Actor::AttachChild(Actor* child)
    {
        if (!child || child == this) return;
        if (IsDescendantOf(child)) return;

        child->DetachFromParent();
        child->m_Parent = this;

        m_Children.push_back(child);
        child->OnAttach(this);
    }

    void Actor::DetachChild(Actor* child)
    {
        if (!child) return;

        auto it = std::find(m_Children.begin(), m_Children.end(), child);
        if (it != m_Children.end()) {
            child->m_Parent = nullptr;
            m_Children.erase(it);

            child->OnDetach(this);
        }
    }

    void Actor::DetachFromParent()
    {
        if (m_Parent) m_Parent->DetachChild(this);
    }

    void Actor::AddComponentRaw(Component* comp)
    {
        if (!comp) return;
        std::type_index idx(typeid(*comp));
        if (m_ComponentMap.count(idx)) return;
        comp->SetOwner(this);
        m_ComponentMap[idx] = comp;
        m_Components.push_back(comp);
        if (m_Initialized)
            comp->OnInit();
    }

    void Actor::RemoveComponentRaw(Component* comp)
    {
        if (!comp) return;
        std::type_index idx(typeid(*comp));
        auto mapIt = m_ComponentMap.find(idx);
        if (mapIt == m_ComponentMap.end() || mapIt->second != comp) return;
        m_Components.erase(std::find(m_Components.begin(), m_Components.end(), comp));
        m_ComponentMap.erase(mapIt);
        delete comp;
    }

    bool Actor::IsDescendantOf(const Actor* actor) const
    {
        if (!actor) return false;

        const Actor* current = m_Parent;
        while (current) {
            if (current == actor) return true;
            current = current->m_Parent;
        }
        return false;
    }

    void Actor::Inspect()
    {
        // Name
        char nameBuf[256];
        strncpy(nameBuf, m_Name.c_str(), sizeof(nameBuf) - 1);
        nameBuf[sizeof(nameBuf) - 1] = '\0';
        ImGui::SetNextItemWidth(-1.0f);
        if (ImGui::InputText("##name", nameBuf, sizeof(nameBuf)))
            m_Name = nameBuf;

        // Active flag
        ImGui::Checkbox("Active", &m_Active);

        ImGui::Separator();

        // Components — each gets a collapsing header and calls its own Inspect()
        Component* compToRemove = nullptr;
        for (auto* comp : m_Components)
        {
            std::string compName =
                ComponentRegistry::Get().GetNameForType(typeid(*comp));
            if (compName.empty())
                compName = "Unknown Component";

            ImGui::PushID(comp);
            UIUtils::PushStylized();
            bool open = ImGui::TreeNodeEx(compName.c_str(), ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed);
            UIUtils::PopStylized();
            if (ImGui::BeginPopupContextItem("##CompCtx")) {
                bool isTransform = dynamic_cast<Transform*>(comp) != nullptr;
                if (ImGui::MenuItem("Delete Component", nullptr, false, !isTransform))
                    compToRemove = comp;
                ImGui::EndPopup();
            }
            if (open) {
                comp->Inspect();
                ImGui::TreePop();
            }
            ImGui::PopID();
        }
        if (compToRemove)
            RemoveComponentRaw(compToRemove);

        ImGui::Separator();

        // Collect types already on this actor
        std::unordered_set<std::type_index> existing;
        for (auto* c : m_Components)
            existing.insert(typeid(*c));

        UIUtils::PushStylized();
        if (UIUtils::Button("Add Component")) {
            static char searchBuf[128] = {};
            searchBuf[0] = '\0';
            ImGui::OpenPopup("##AddComponent");
        }
        UIUtils::PopStylized();

        if (ImGui::BeginPopup("##AddComponent")) {
            static char searchBuf[128] = {};
            ImGui::SetNextItemWidth(-1.0f);
            if (ImGui::IsWindowAppearing()) {
                searchBuf[0] = '\0';
                ImGui::SetKeyboardFocusHere();
            }
            ImGui::InputText("##CompSearch", searchBuf, sizeof(searchBuf));

            ImGui::Separator();

            std::string filter = searchBuf;
            std::transform(filter.begin(), filter.end(), filter.begin(),
                [](unsigned char c) { return std::tolower(c); });

            std::type_index selectedType = typeid(void);
            ComponentRegistry::Get().ForEach([&](const ComponentRegistry::Entry& entry) {
                if (existing.count(entry.Type))
                    return true;
                if (!filter.empty()) {
                    std::string name = entry.Name;
                    std::transform(name.begin(), name.end(), name.begin(),
                        [](unsigned char c) { return std::tolower(c); });
                    if (name.find(filter) == std::string::npos)
                        return true;
                }
                if (ImGui::MenuItem(entry.Name.c_str()))
                    selectedType = entry.Type;
                return true;
            });
            ImGui::EndPopup();

            if (selectedType != typeid(void)) {
                auto* comp = ComponentRegistry::Get().CreateByType(selectedType, this);
                if (comp)
                    AddComponentRaw(comp);
            }
        }
    }
}
