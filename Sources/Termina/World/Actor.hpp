#pragma once

#include <vector>
#include <typeindex>
#include <unordered_map>
#include <type_traits>
#include <algorithm>

#include <Termina/Core/Common.hpp>
#include <Termina/Core/ID.hpp>
#include <Termina/World/Component.hpp>

namespace Termina {
    class World;

    class Actor
    {
    public:
        Actor(World* parentWorld, const std::string& name = "Entity");
        virtual ~Actor();

        template<typename T, typename... Args>
        T& AddComponent(Args&&... args)
        {
            static_assert(std::is_base_of_v<Component, T>);
            // TODO: static_assert if component is not in registry
            T* ptr = new T(this, std::forward<Args>(args)...);
            ptr->SetOwner(this);
            m_ComponentMap[typeid(T)] = ptr;
            m_Components.push_back(std::move(ptr));
            return *ptr;
        }

        template<typename T>
        T& GetComponent() const
        {
            auto it = m_ComponentMap.find(typeid(T));
            return *static_cast<T*>(it->second);
        }

        template<typename T>
        bool HasComponent() const
        {
            auto it = m_ComponentMap.find(typeid(T));
            return (it != m_ComponentMap.end()) ? true : false;
        }

        template<typename T>
        void RemoveComponent()
        {
            if (HasComponent<T>()) {
                auto it = m_ComponentMap.find(typeid(T));
                m_Components.erase(std::find(m_Components.begin(), m_Components.end(), it->second));
                delete it->second;
                m_ComponentMap.erase(it);
            }
        }

        void OnInit();
        void OnShutdown();
        void OnPlay();
        void OnStop();

        void OnPreUpdate(float deltaTime);
        void OnUpdate(float deltaTime);
        void OnPostUpdate(float deltaTime);
        void OnPrePhysics(float deltaTime);
        void OnPhysics(float deltaTime);
        void OnPostPhysics(float deltaTime);
        void OnPreRender(float deltaTime);
        void OnRender(float deltaTime);
        void OnPostRender(float deltaTime);
        void OnAttach(Actor* newParent);
        void OnDetach(Actor* oldParent);

        void AttachChild(Actor* child);
        void DetachChild(Actor* child);
        void DetachFromParent();
        bool IsDescendantOf(const Actor* actor) const;

        Actor* GetParent() const { return m_Parent; }
        const std::vector<Actor*>& GetChildren() const { return m_Children; }

        bool IsActive() const { return m_Active; }
        void SetActive(bool a) { m_Active = a; }

        std::string GetName() const { return m_Name; }
        void SetName(const std::string& name) { m_Name = name; }

        std::vector<Component*>& GetAllComponents() { return m_Components; }

        // Insert a pre-created component (e.g., from ComponentRegistry::CreateByName).
        // Takes ownership. Does nothing if a component of the same type is already present.
        void AddComponentRaw(Component* comp);

        // Remove and delete a component by pointer. Does nothing if not found.
        void RemoveComponentRaw(Component* comp);

        uint64 GetID() const { return m_ID; }
        World* GetParentWorld() { return m_ParentWorld; }

    private:
        friend class World;
        void SetID(uint64 id) { m_ID = id; }

        World* m_ParentWorld;

        std::string m_Name;
        bool m_Active = true;

        std::vector<Component*> m_Components;
        std::unordered_map<std::type_index, Component*> m_ComponentMap;

        Actor* m_Parent = nullptr;
        std::vector<Actor*> m_Children;
        uint64 m_ID = NewID();

        bool m_InPlayMode = false;
        bool m_Initialized = false;
    };
}
