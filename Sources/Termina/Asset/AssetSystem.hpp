#pragma once

#include "AssetHandle.hpp"
#include "IAssetLoader.hpp"

#include <Termina/Core/System.hpp>

#include <functional>
#include <memory>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <vector>

namespace Termina {

    class AssetSystem : public ISystem
    {
    public:
        AssetSystem();
        ~AssetSystem();

        // -----------------------------------------------------------------
        // ISystem
        // -----------------------------------------------------------------

        /// Calls ProcessPendingDeletions() — hooked into PostRender so assets are
        /// freed after the GPU has finished using them.
        void PostRender(float deltaTime) override;

        UpdateFlags GetUpdateFlags() const override { return UpdateFlags::RenderUpdateDuringEditor; }
        std::string GetName()        const override { return "Asset System"; }
        int         GetPriority()    const override { return 100; }

        // -----------------------------------------------------------------
        // Loaders
        // -----------------------------------------------------------------

        /// Register a loader for asset type T. AssetSystem does not own the loader.
        template<typename T>
        void RegisterLoader(IAssetLoader<T>* loader)
        {
            m_Loaders[std::type_index(typeid(T))] = std::make_unique<TypedLoader<T>>(loader);
        }

        // -----------------------------------------------------------------
        // Loading
        // -----------------------------------------------------------------

        /// Load an asset by path, or return the cached handle if already loaded.
        /// Returns an invalid handle on failure — check IsValid() or fall back to GetDefault<T>().
        template<typename T>
        AssetHandle<T> Load(const std::string& path)
        {
            auto pathIt = m_PathToID.find(path);
            if (pathIt != m_PathToID.end())
            {
                uint32 id = pathIt->second;
                m_Records[id].RefCount++;
                return AssetHandle<T>(id, this);
            }

            auto loaderIt = m_Loaders.find(std::type_index(typeid(T)));
            if (loaderIt == m_Loaders.end())
                return AssetHandle<T>{};

            T* asset = static_cast<T*>(loaderIt->second->LoadFromDisk(path));
            if (!asset)
                return AssetHandle<T>{};

            uint32 id = m_NextID++;
            AssetRecord record;
            record.Asset    = asset;
            record.RefCount = 1;
            record.Path     = path;
            record.Deleter  = [](void* ptr) { delete static_cast<T*>(ptr); };
            m_Records[id]   = std::move(record);
            m_PathToID[path] = id;

            return AssetHandle<T>(id, this);
        }

        // -----------------------------------------------------------------
        // Defaults
        // -----------------------------------------------------------------

        /// Set the default (fallback) asset for type T. AssetSystem takes ownership.
        template<typename T>
        void SetDefault(T* asset)
        {
            auto key = std::type_index(typeid(T));
            auto it  = m_Defaults.find(key);
            if (it != m_Defaults.end())
                it->second.second(it->second.first);
            m_Defaults[key] = { asset, [](void* ptr) { delete static_cast<T*>(ptr); } };
        }

        /// Returns the default asset for type T, or nullptr if none is set.
        template<typename T>
        T* GetDefault() const
        {
            auto it = m_Defaults.find(std::type_index(typeid(T)));
            if (it == m_Defaults.end())
                return nullptr;
            return static_cast<T*>(it->second.first);
        }

        // -----------------------------------------------------------------
        // Lifetime management
        // -----------------------------------------------------------------

        /// Evict assets whose refCount <= minRefCount into the pending-deletion queue.
        /// Does NOT free memory — call ProcessPendingDeletions() after the GPU is done.
        void Clean(int32 minRefCount = 0);

        /// Free all queued assets. Call once it is safe to release GPU resources
        /// (e.g. after waiting on the previous frame's fence).
        void ProcessPendingDeletions();

        // -----------------------------------------------------------------
        // Called by AssetHandle — not for direct use
        // -----------------------------------------------------------------
        void AddRef(uint32 id);
        void Release(uint32 id);

        template<typename T>
        T* GetRaw(uint32 id) const
        {
            auto it = m_Records.find(id);
            if (it == m_Records.end())
                return nullptr;
            return static_cast<T*>(it->second.Asset);
        }

    private:
        struct AssetRecord
        {
            void*                      Asset    = nullptr;
            int32                      RefCount = 0;
            std::string                Path;
            std::function<void(void*)> Deleter;
        };

        struct PendingDelete
        {
            void*                      Asset;
            std::function<void(void*)> Deleter;
        };

        struct LoaderBase
        {
            virtual ~LoaderBase() = default;
            virtual void* LoadFromDisk(const std::string& path) = 0;
            virtual void* LoadFromPackage(const uint8* data, size_t size) = 0;
        };

        template<typename T>
        struct TypedLoader : LoaderBase
        {
            explicit TypedLoader(IAssetLoader<T>* loader) : m_Loader(loader) {}

            void* LoadFromDisk(const std::string& path) override
            {
                return m_Loader->LoadFromDisk(path);
            }

            void* LoadFromPackage(const uint8* data, size_t size) override
            {
                return m_Loader->LoadFromPackage(data, size);
            }

            IAssetLoader<T>* m_Loader;
        };

        void RegisterDefaults();

        uint32 m_NextID = 1;
        bool   m_DefaultsRegistered = false;

        std::unordered_map<uint32, AssetRecord>                              m_Records;
        std::unordered_map<std::string, uint32>                              m_PathToID;
        std::unordered_map<std::type_index, std::unique_ptr<LoaderBase>>     m_Loaders;
        std::unordered_map<std::type_index,
            std::pair<void*, std::function<void(void*)>>>                    m_Defaults;
        std::vector<PendingDelete>                                           m_PendingDeletion;

        // Owned loader instances — forward-declared to avoid circular includes
        // (TextureLoader.hpp includes AssetSystem.hpp).
        std::unique_ptr<class TextureLoader> m_TextureLoader;
    };

    // -------------------------------------------------------------------------
    // AssetHandle<T> method bodies — defined here because they require the full
    // AssetSystem definition above.
    // -------------------------------------------------------------------------

    template<typename T>
    AssetHandle<T>::AssetHandle(const AssetHandle& other)
        : m_ID(other.m_ID), m_System(other.m_System)
    {
        IncRef();
    }

    template<typename T>
    AssetHandle<T>& AssetHandle<T>::operator=(const AssetHandle& other)
    {
        if (this != &other)
        {
            Reset();
            m_ID     = other.m_ID;
            m_System = other.m_System;
            IncRef();
        }
        return *this;
    }

    template<typename T>
    AssetHandle<T>::AssetHandle(AssetHandle&& other) noexcept
        : m_ID(other.m_ID), m_System(other.m_System)
    {
        other.m_ID     = 0;
        other.m_System = nullptr;
    }

    template<typename T>
    AssetHandle<T>& AssetHandle<T>::operator=(AssetHandle&& other) noexcept
    {
        if (this != &other)
        {
            Reset();
            m_ID     = other.m_ID;
            m_System = other.m_System;
            other.m_ID     = 0;
            other.m_System = nullptr;
        }
        return *this;
    }

    template<typename T>
    T* AssetHandle<T>::Get() const
    {
        if (!m_System || m_ID == 0)
            return nullptr;
        return m_System->GetRaw<T>(m_ID);
    }

    template<typename T>
    void AssetHandle<T>::IncRef()
    {
        if (m_System && m_ID)
            m_System->AddRef(m_ID);
    }

    template<typename T>
    void AssetHandle<T>::Reset()
    {
        if (m_System && m_ID)
            m_System->Release(m_ID);
        m_ID     = 0;
        m_System = nullptr;
    }

} // namespace Termina
