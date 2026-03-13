#pragma once

#include <memory>
#include <string>

#include <Termina/Core/System.hpp>
#include "World.hpp"

namespace Termina {
    /// Manages the current world, including loading, saving, and transitioning between worlds.
    class WorldSystem : public ISystem {
    public:
        WorldSystem();
        ~WorldSystem();

        // Lifecycle
        void PreUpdate(float deltaTime) override;
        void Update(float deltaTime) override;
        void PostUpdate(float deltaTime) override;
        void PrePhysics(float deltaTime) override;
        void Physics(float deltaTime) override;
        void PostPhysics(float deltaTime) override;
        void PreRender(float deltaTime) override;
        void Render(float deltaTime) override;
        void PostRender(float deltaTime) override;

        void RegisterComponents() override;
        void UnregisterComponents() override;

        // World management
        World* GetCurrentWorld() const { return m_CurrentWorld.get(); }

        // Create a blank world, replacing the current one.
        void NewWorld(const std::string& name = "World");

        // Load a world from disk, replacing the current one.
        // Returns true on success; on failure the previous world is preserved.
        bool LoadWorld(const std::string& path);

        // Save the current world. Uses the world's existing path when no
        // argument is supplied. Returns true on success.
        bool SaveWorld(const std::string& path = "");

        // Play / stop forwarded to the current world.
        void Play();
        void Stop();
        bool IsPlaying() const { return m_IsPlaying; }

        UpdateFlags GetUpdateFlags() const override { return UpdateFlags::UpdateDuringEditor | UpdateFlags::RenderUpdateDuringEditor | UpdateFlags::PhysicsUpdateDuringEditor; }
        std::string GetName() const override { return "World System"; }
        int GetPriority() const override { return 0; }

    private:
        // Cleanly shuts down the current world and takes ownership of newWorld.
        void TransitionTo(std::unique_ptr<World> newWorld);

        std::unique_ptr<World> m_CurrentWorld;
        bool m_IsPlaying = false;

        std::string m_PrePlayPath;    // world path before entering play mode
        std::string m_PlaySnapshot;   // temp file used to snapshot world state on play
    };
}
