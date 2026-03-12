#pragma once

#include "World/Components/Transform.hpp"

#include <Termina/World/World.hpp>

namespace TerminaScript {
    class ScriptableComponent : public Termina::Component
    {
    public:
        virtual void Awake() {}
        virtual void Start() {}
        virtual void Stop() {}
        virtual void PreUpdate(float deltaTime) {}
        virtual void Update(float deltaTime) {}
        virtual void PostUpdate(float deltaTime) {}
        virtual void PrePhysics(float deltaTime) {}
        virtual void Physics(float deltaTime) {}
        virtual void PostPhysics(float deltaTime) {}

        // TODO: TriggerEnter, TriggerStay, TriggerExit, CollisionEnter, CollisionStay, CollisionExit

    protected:
        void Instantiate(Termina::Actor* actor);
        void Destroy(Termina::Actor* actor);

        Termina::Transform* m_Transform;
        std::string m_Name;
    
        template<typename T>
        T* AddComponent()
        {
            return m_Owner->AddComponent<T>();
        }

        template<typename T>
        T* GetComponent()
        {
            return m_Owner->GetComponent<T>();
        }

        template<typename T>
        T* RemoveComponent()
        {
            return m_Owner->RemoveComponent<T>();
        }

        template<typename T>
        bool HasComponent()
        {
            return m_Owner->HasComponent<T>();
        }

        std::vector<Termina::Actor*> GetChildren()
        {
            return m_Owner->GetChildren();
        }
    };
}
