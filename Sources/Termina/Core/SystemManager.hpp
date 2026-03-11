#pragma once

#include "System.h"

#include <typeindex>
#include <unordered_map>
#include <vector>

namespace Termina {
    class SystemManager {
    public:
        SystemManager() = default;
        ~SystemManager();

        template<typename T, typename... Args>
        T* AddSystem(Args&&... args)
        {
            T* system = new T(std::forward<Args>(args)...);
            m_Subsystems[typeid(T)] = system;
            return system;
        }

        template<typename T>
        T* GetSystem()
        {
            return m_Subsystems[typeid(T)];
        }
        
        void Begin();
        void Clean();

        void PreUpdate(float deltaTime);
        void Update(float deltaTime);
        void PostUpdate(float deltaTime);
        void PrePhysics(float deltaTime);
        void Physics(float deltaTime);
        void PostPhysics(float deltaTime);
        void PreRender(float deltaTime);
        void Render(float deltaTime);
        void PostRender(float deltaTime);

        void SetIsInEditor(bool isInEditor);
    private:
        std::unordered_map<std::type_index, ISystem*> m_Subsystems;
        std::vector<ISystem*> m_UpdateList;
        
        bool m_IsInEditor = false;
    };
}
