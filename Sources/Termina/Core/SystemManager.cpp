#include "SystemManager.hpp"

#include <algorithm>

namespace Termina {
    SystemManager::~SystemManager()
    {
        Clean();
    }

    void SystemManager::Clean()
    {
        for (auto [_, system] : m_Subsystems) {
            delete system;
        }
        m_Subsystems.clear();
    }

    void SystemManager::Begin()
    {
        // Create list of systems sorted on priority
        for (auto [_, system] : m_Subsystems) {
            m_UpdateList.push_back(system);
        }
        std::sort(m_UpdateList.begin(), m_UpdateList.end(), [](const auto& a, const auto& b) {
            return a->GetPriority() < b->GetPriority();
        });
    }

    void SystemManager::PreUpdate(float deltaTime)
    {
        for (ISystem* system : m_UpdateList) {
            if (m_IsInEditor && Any(system->GetUpdateFlags(), UpdateFlags::UpdateDuringEditor)) system->PreUpdate(deltaTime);
            else if (!m_IsInEditor) system->PreUpdate(deltaTime);
        }
    }

    void SystemManager::Update(float deltaTime)
    {
        for (ISystem* system : m_UpdateList) {
            if (m_IsInEditor && Any(system->GetUpdateFlags(), UpdateFlags::UpdateDuringEditor)) system->Update(deltaTime);
            else if (!m_IsInEditor) system->Update(deltaTime);
        }
    }

    void SystemManager::PostUpdate(float deltaTime)
    {
        for (ISystem* system : m_UpdateList) {
            if (m_IsInEditor && Any(system->GetUpdateFlags(), UpdateFlags::UpdateDuringEditor)) system->PostUpdate(deltaTime);
            else if (!m_IsInEditor) system->PostUpdate(deltaTime);
        }
    }

    void SystemManager::PrePhysics(float deltaTime)
    {
        for (ISystem* system : m_UpdateList) {
            if (m_IsInEditor && Any(system->GetUpdateFlags(), UpdateFlags::PhysicsUpdateDuringEditor)) system->PrePhysics(deltaTime);
            else if (!m_IsInEditor) system->PrePhysics(deltaTime);
        }
    }

    void SystemManager::Physics(float deltaTime)
    {
        for (ISystem* system : m_UpdateList) {
            if (m_IsInEditor && Any(system->GetUpdateFlags(), UpdateFlags::PhysicsUpdateDuringEditor)) system->Physics(deltaTime);
            else if (!m_IsInEditor) system->Physics(deltaTime);
        }
    }

    void SystemManager::PostPhysics(float deltaTime)
    {
        for (ISystem* system : m_UpdateList) {
            if (m_IsInEditor && Any(system->GetUpdateFlags(), UpdateFlags::PhysicsUpdateDuringEditor)) system->PostPhysics(deltaTime);
            else if (!m_IsInEditor) system->PostPhysics(deltaTime);
        }
    }

    void SystemManager::PreRender(float deltaTime)
    {
        for (ISystem* system : m_UpdateList) {
            if (m_IsInEditor && Any(system->GetUpdateFlags(), UpdateFlags::RenderUpdateDuringEditor)) system->PreRender(deltaTime);
            else if (!m_IsInEditor) system->PreRender(deltaTime);
        }
    }

    void SystemManager::Render(float deltaTime)
    {
        for (ISystem* system : m_UpdateList) {
            if (m_IsInEditor && Any(system->GetUpdateFlags(), UpdateFlags::RenderUpdateDuringEditor)) system->Render(deltaTime);
            else if (!m_IsInEditor) system->Render(deltaTime);
        }
    }

    void SystemManager::PostRender(float deltaTime)
    {
        for (ISystem* system : m_UpdateList) {
            if (m_IsInEditor && Any(system->GetUpdateFlags(), UpdateFlags::RenderUpdateDuringEditor)) system->PostRender(deltaTime);
            else if (!m_IsInEditor) system->PostRender(deltaTime);
        }
    }

    void SystemManager::SetIsInEditor(bool isInEditor)
    {
        m_IsInEditor = isInEditor;
    }
}
