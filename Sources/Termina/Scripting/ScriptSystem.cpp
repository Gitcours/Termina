#include "ScriptSystem.hpp"
#include "Core/Application.hpp"
#include "Core/FileSystem.hpp"
#include "Platform/LaunchProcess.hpp"
#include "Scripting/ScriptModuleManager.hpp"
#include "World/WorldSystem.hpp"

namespace Termina {
    ScriptSystem::ScriptSystem()
    {
        ScriptModuleManager::Get().Load("Game", "libGame.so");
        RebuildWatches();
    }

    ScriptSystem::~ScriptSystem()
    {

    }

    void ScriptSystem::Compile()
    {
        LaunchProcess::Launch("xmake build Game", {});
    }

    void ScriptSystem::Recompile()
    {
        LaunchProcess::Launch("xmake clean Game", {});
        LaunchProcess::Launch("xmake build Game", {});
    }

    void ScriptSystem::RebuildWatches()
    {
        m_Watches.clear();
        for (auto& file : FileSystem::GetFilesRecursive("Sources/Game")) {
            if (FileSystem::HasExtension(file, ".cpp") || FileSystem::HasExtension(file, ".hpp"))
                m_Watches.push_back(FileSystem::WatchFile(file));
        }
    }

    void ScriptSystem::Update(float deltaTime)
    {
        // Execute reload at start of frame (priority -1 ensures we run before WorldSystem)
        // so no component list is being iterated when we add/remove components.
        if (m_PendingReload) {
            m_PendingReload = false;
            Compile();
            World* world = Application::GetSystem<WorldSystem>()->GetCurrentWorld();
            ScriptModuleManager::Get().Reload("Game", world);
            RebuildWatches();
            return;
        }

        for (const auto& watch : m_Watches) {
            if (FileSystem::HasFileChanged(watch)) {
                m_PendingReload = true;
                return;
            }
        }
    }
}
