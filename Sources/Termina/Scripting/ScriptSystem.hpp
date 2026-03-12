#pragma once

#include "ScriptModuleManager.hpp"

#include <Termina/Core/System.hpp>
#include <Termina/Core/FileSystem.hpp>

namespace Termina {
    class ScriptSystem : public ISystem
    {
    public:
        ScriptSystem();
        ~ScriptSystem();

        void Compile();
        void Recompile();
        void Update(float deltaTime) override;

        UpdateFlags GetUpdateFlags() const override { return UpdateFlags::UpdateDuringEditor; }
        std::string GetName() const override { return "Script System"; }
        int GetPriority() const override { return -1; }

    private:
        void RebuildWatches();

        std::vector<FileSystem::Watch> m_Watches;
        bool m_PendingReload = false;
    };
}
