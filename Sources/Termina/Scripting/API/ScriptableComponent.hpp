#pragma once

#include <Termina/World/World.hpp>

namespace TerminaScript {
    class ScriptableComponent : public Termina::Component
    {
    public:
        ScriptableComponent() = default;
        ScriptableComponent(Termina::Actor* owner) : Termina::Component(owner) {}

        virtual void Awake() {}
        virtual void Start() {}
        virtual void Stop() {}
        virtual void PreUpdate(float deltaTime) {}
        virtual void Update(float deltaTime) {}
        virtual void PostUpdate(float deltaTime) {}
        virtual void PrePhysics(float deltaTime) {}
        virtual void Physics(float deltaTime) {}
        virtual void PostPhysics(float deltaTime) {}

        // Override to persist fields across hot-reloads and world saves.
        virtual void Serialize(nlohmann::json& out) const override {}
        virtual void Deserialize(const nlohmann::json& in) override {}

        // TODO: TriggerEnter, TriggerStay, TriggerExit, CollisionEnter, CollisionStay, CollisionExit

        Termina::UpdateFlags GetUpdateFlags() const override { return (Termina::UpdateFlags)0; }

    private:
        void OnInit()                override;
        void OnPlay()                override { Start(); }
        void OnStop()                override { Stop(); }
        void OnPreUpdate(float dt)   override { PreUpdate(dt); }
        void OnUpdate(float dt)      override { Update(dt); }
        void OnPostUpdate(float dt)  override { PostUpdate(dt); }
        void OnPrePhysics(float dt)  override { PrePhysics(dt); }
        void OnPhysics(float dt)     override { Physics(dt); }
        void OnPostPhysics(float dt) override { PostPhysics(dt); }

    protected:
        friend class ScriptManager;

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
