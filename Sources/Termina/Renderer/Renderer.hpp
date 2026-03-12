#pragma once

#include <Termina/Core/System.hpp>
#include <Termina/Core/Window.hpp>
#include <Termina/RHI/Device.hpp>

#include "RenderPass.hpp"
#include "GPUBumpAllocator.h"
#include "TemporaryContext.h"
#include "GPUUploader.h"
#include "ResourceViewCache.h"
#include "SamplerCache.h"
#include "PassIO.hpp"
#include "Camera.hpp"

namespace Termina {
    using RenderCallback = std::function<void(RendererDevice*, RendererSurface*, float)>;

    class RendererSystem : public ISystem
    {
    public:
        RendererSystem(Window* window);
        ~RendererSystem();

        void PreUpdate(float deltaTime) override;
        void PreRender(float deltaTime) override;
        void Render(float deltaTime) override;

        void RegisterRenderCallback(const RenderCallback& callback) { m_RenderCallbacks.push_back(callback); }
        void SetCurrentCamera(const Camera& camera) { m_CurrentCamera = camera; }
        const Camera& GetCurrentCamera() const { return m_CurrentCamera; }

        UpdateFlags GetUpdateFlags() const override {
            return UpdateFlags::RenderUpdateDuringEditor | UpdateFlags::UpdateDuringEditor;
        }
        std::string GetName() const override { return "Renderer System"; }
        int GetPriority() const override { return 0; }

        RendererDevice* GetDevice() const { return m_Device; }
        RendererSurface* GetSurface() const { return m_Surface; }
        GPUBumpAllocator* GetGPUAllocator() const { return m_GPUAllocator; }
        TemporaryContext* GetTemporaryContext() const { return m_TemporaryContext; }
        GPUUploader* GetGPUUploader() const { return m_GPUUploader; }
        ResourceViewCache* GetResourceViewCache() const { return m_ResourceViewCache; }
        SamplerCache* GetSamplerCache() const { return m_SamplerCache; }
        PassIO* GetPassIO() { return &m_PassIO; }
    private:
        void BakeTimeline();

        Window* m_Window;
	    RendererDevice* m_Device = nullptr;
	    RendererSurface* m_Surface = nullptr;

        Camera m_CurrentCamera;

        int m_CurrentWidth = 0;
        int m_CurrentHeight = 0;
        bool m_BakedTimeline = false;

        std::vector<RenderCallback> m_RenderCallbacks;
        std::vector<RenderPass*> m_RenderPasses;

        GPUBumpAllocator* m_GPUAllocator = nullptr;
        TemporaryContext* m_TemporaryContext = nullptr;
        GPUUploader* m_GPUUploader = nullptr;
        ResourceViewCache* m_ResourceViewCache = nullptr;
        SamplerCache* m_SamplerCache = nullptr;
        PassIO m_PassIO;
    };
}
