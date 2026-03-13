#pragma once

#include <vector>
#include <memory>
#include <string>

#include <Termina/Renderer/Camera.hpp>

#include "Actor.hpp"
#include "Components/Transform.hpp"

namespace Termina {
    class World
    {
    public:
        ~World();

        /// Spawns a new actor with a transform component.
        Actor* SpawnActor();
        /// Spawns a new actor from an existing actor, copying its components.
        Actor* SpawnActorFrom(Actor* actor);
        /// Destroys an actor, removing it from the world.
        void DestroyActor(Actor* actor);
        /// Returns an actor by its unique ID.
        Actor* GetActorById(uint64 id);
        /// Returns an actor by its name.
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
        void SetCurrentPath(const std::string& path) { m_CurrentPath = path; }

        void SetMainCamera(Actor* camera) { m_MainCamera = camera; }
        Camera GetMainCamera() const;

        void SetAudioListener(Actor* listener) { m_AudioListener = listener; }
        Actor* GetAudioListener() const { return m_AudioListener; }
    private:
        std::vector<std::shared_ptr<Actor>> m_Actors;

        Actor* m_MainCamera = nullptr;
        Actor* m_AudioListener = nullptr;

        std::string m_CurrentPath = "";
        std::string m_Name = "World";
    };
}
