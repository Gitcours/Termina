#include "Actor.hpp"
#include "Component.hpp"

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
}
