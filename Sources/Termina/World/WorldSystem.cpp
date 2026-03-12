#include "WorldSystem.hpp"

#include <stdexcept>

#include "ComponentRegistry.hpp"
#include "Components/Transform.hpp"

namespace Termina {

    WorldSystem::WorldSystem()
    {
        // Start with an empty default world so GetCurrentWorld() is never null.
        m_CurrentWorld = std::make_unique<World>();
        m_CurrentWorld->OnInit();
    }

    WorldSystem::~WorldSystem()
    {
        if (m_IsPlaying)
            m_CurrentWorld->OnStop();
        m_CurrentWorld->OnShutdown();
    }

    // ---------------------------------------------------------------------------
    // World management
    // ---------------------------------------------------------------------------

    void WorldSystem::TransitionTo(std::unique_ptr<World> newWorld)
    {
        if (m_CurrentWorld)
        {
            if (m_IsPlaying)
            {
                m_CurrentWorld->OnStop();
                m_IsPlaying = false;
            }
            m_CurrentWorld->OnShutdown();
        }
        m_CurrentWorld = std::move(newWorld);
    }

    void WorldSystem::NewWorld(const std::string& name)
    {
        auto world = std::make_unique<World>();
        world->SetName(name);
        TransitionTo(std::move(world));
        m_CurrentWorld->OnInit();
    }

    bool WorldSystem::LoadWorld(const std::string& path)
    {
        auto candidate = std::make_unique<World>();
        try
        {
            candidate->LoadFromFile(path);
        }
        catch (const std::exception&)
        {
            // Leave the current world intact.
            return false;
        }

        TransitionTo(std::move(candidate));
        return true;
    }

    bool WorldSystem::SaveWorld(const std::string& path)
    {
        if (!m_CurrentWorld)
            return false;
        try
        {
            m_CurrentWorld->SaveToFile(path);
        }
        catch (const std::exception&)
        {
            return false;
        }
        return true;
    }

    void WorldSystem::Play()
    {
        if (!m_CurrentWorld || m_IsPlaying)
            return;
        m_IsPlaying = true;
        m_CurrentWorld->OnPlay();
    }

    void WorldSystem::Stop()
    {
        if (!m_CurrentWorld || !m_IsPlaying)
            return;
        m_CurrentWorld->OnStop();
        m_IsPlaying = false;
    }

    // ---------------------------------------------------------------------------
    // ISystem lifecycle forwarded to the current world
    // ---------------------------------------------------------------------------

    void WorldSystem::PreUpdate(float deltaTime)
    {
        if (m_CurrentWorld)
            m_CurrentWorld->OnPreUpdate(deltaTime);
    }

    void WorldSystem::Update(float deltaTime)
    {
        if (m_CurrentWorld)
            m_CurrentWorld->OnUpdate(deltaTime);
    }

    void WorldSystem::PostUpdate(float deltaTime)
    {
        if (m_CurrentWorld)
            m_CurrentWorld->OnPostUpdate(deltaTime);
    }

    void WorldSystem::PrePhysics(float deltaTime)
    {
        if (m_CurrentWorld)
            m_CurrentWorld->OnPrePhysics(deltaTime);
    }

    void WorldSystem::Physics(float deltaTime)
    {
        if (m_CurrentWorld)
            m_CurrentWorld->OnPhysics(deltaTime);
    }

    void WorldSystem::PostPhysics(float deltaTime)
    {
        if (m_CurrentWorld)
            m_CurrentWorld->OnPostPhysics(deltaTime);
    }

    void WorldSystem::PreRender(float deltaTime)
    {
        if (m_CurrentWorld)
            m_CurrentWorld->OnPreRender(deltaTime);
    }

    void WorldSystem::Render(float deltaTime)
    {
        if (m_CurrentWorld)
            m_CurrentWorld->OnRender(deltaTime);
    }

    void WorldSystem::PostRender(float deltaTime)
    {
        if (m_CurrentWorld)
            m_CurrentWorld->OnPostRender(deltaTime);
    }

    // ---------------------------------------------------------------------------
    // Component registration
    // ---------------------------------------------------------------------------

    void WorldSystem::RegisterComponents()
    {
        ComponentRegistry::Get().Register<Transform>("Transform");
    }

    void WorldSystem::UnregisterComponents()
    {
        ComponentRegistry::Get().Unregister<Transform>();
    }
}
