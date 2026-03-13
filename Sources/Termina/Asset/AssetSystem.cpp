#include "AssetSystem.hpp"
#include "Texture/TextureLoader.hpp"

#include <vector>

namespace Termina {
    AssetSystem::AssetSystem()
    {
        m_TextureLoader = std::make_unique<TextureLoader>();
        RegisterLoader<TextureAsset>(m_TextureLoader.get());
    }

    AssetSystem::~AssetSystem()
    {
        ProcessPendingDeletions();
        for (auto& [id, record] : m_Records)
            record.Deleter(record.Asset);
        for (auto& [type, defaultEntry] : m_Defaults)
            defaultEntry.second(defaultEntry.first);
    }

    void AssetSystem::AddRef(uint32 id)
    {
        auto it = m_Records.find(id);
        if (it != m_Records.end())
            it->second.RefCount++;
    }

    void AssetSystem::Release(uint32 id)
    {
        auto it = m_Records.find(id);
        if (it != m_Records.end() && it->second.RefCount > 0)
            it->second.RefCount--;
    }

    void AssetSystem::Clean(int32 minRefCount)
    {
        std::vector<uint32> toEvict;
        for (auto& [id, record] : m_Records)
        {
            if (record.RefCount <= minRefCount)
                toEvict.push_back(id);
        }

        for (uint32 id : toEvict)
        {
            auto& record = m_Records[id];
            m_PendingDeletion.push_back({ record.Asset, record.Deleter });
            m_PathToID.erase(record.Path);
            m_Records.erase(id);
        }
    }

    void AssetSystem::PostRender(float /*deltaTime*/)
    {
        if (!m_DefaultsRegistered)
        {
            RegisterDefaults();
            m_DefaultsRegistered = true;
        }
        ProcessPendingDeletions();
    }

    void AssetSystem::RegisterDefaults()
    {
        m_TextureLoader->RegisterDefault(*this);
    }

    void AssetSystem::ProcessPendingDeletions()
    {
        for (auto& entry : m_PendingDeletion)
            entry.Deleter(entry.Asset);
        m_PendingDeletion.clear();
    }

} // namespace Termina
