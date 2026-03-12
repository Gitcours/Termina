#pragma once

#include <vector>
#include <memory>
#include <string>

#include "Actor.hpp"
#include "Components/Transform.hpp"

namespace Termina {
    class World
    {
    public:
        ~World();

        Actor* SpawnActor();
        Actor* SpawnActorFrom(Actor* actor);
        void DestroyActor(Actor* actor);
        Actor* GetActorById(uint64 id);
        Actor* GetActorByName(const std::string& name);

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

        void Clear();

        void LoadFromFile(const std::string& filename);
        void SaveToFile(const std::string& filename = "");

        const std::vector<std::shared_ptr<Actor>>& GetActors() const { return m_Actors; }
        std::vector<Actor*> GetRootActors() const;

        const std::string& GetName() const { return m_Name; }
        void SetName(const std::string& name) { m_Name = name; }

        const std::string& GetCurrentPath() const { return m_CurrentPath; }
    private:
        std::vector<std::shared_ptr<Actor>> m_Actors;

        std::string m_CurrentPath = "";
        std::string m_Name = "World";
    };
}
