#pragma once

#include <Termina/Core/Common.hpp>
#include <Termina/RHI/Device.hpp>

#include <vulkan/vulkan.hpp>
#include <VMA/vk_mem_alloc.hpp>

#include "VulkanBindlessManager.hpp"

namespace Termina {
    class VulkanDevice : public RendererDevice
    {
    public:
        VulkanDevice();
        ~VulkanDevice();

        void ExecuteRenderContext(RenderContext* context) override;
        void WaitIdle() override;

        RendererBackend GetBackend() const override { return RendererBackend::Vulkan; }
        RendererSurface* CreateSurface(Window* window) override;
        RenderContext* CreateRenderContext(bool singleTime) override;
        RendererTexture* CreateTexture(const TextureDesc& desc) override;
        TextureView* CreateTextureView(const TextureViewDesc& desc) override;
        RenderPipeline* CreateRenderPipeline(const RenderPipelineDesc& desc) override;
        RendererBuffer* CreateBuffer(const BufferDesc& desc) override;
        BufferView* CreateBufferView(const BufferViewDesc& desc) override;
        Sampler* CreateSampler(const SamplerDesc& desc) override;
        ComputePipeline* CreateComputePipeline(const ShaderModule& module, const std::string& name = "Compute Pipeline") override;

        vk::Instance GetVulkanInstance() const { return m_Instance; }
        vk::Device GetVulkanDevice() const { return m_Device; }
        vk::PhysicalDevice GetVulkanPhysicalDevice() const { return m_PhysicalDevice; }
        vk::Queue GetMainQueue() const { return m_MainQueue; }
        uint32 GetMainQueueFamilyIndex() const { return m_MainQueueFamilyIndex; }
        vk::CommandPool GetCommandPool() const { return m_CommandPool; }
        vma::Allocator GetVulkanAllocator() const { return m_Allocator; }
        VulkanBindlessManager* GetBindlessManager() { return m_BindlessManager; }

        bool SupportsRaytracing() const override { return m_SupportRaytracing; }
        bool SupportsMeshShaders() const override { return m_SupportMeshShaders; }
        uint64 GetOptimalRowPitchAlignment() const override { return m_OptimalRowPitchAlignment; }
        uint64 GetBufferImageGranularity() const override { return m_BufferImageGranularity; }
        TextureFormat GetSurfaceFormat() const override { return TextureFormat::BGRA8_UNORM; }

    private:
        void CreateInstance();
        void CreatePhysicalDevice();
        void CreateDevice();
        void CreateCommandPool();
        void CreateAllocator();

    private:
        vk::Instance m_Instance;
        vk::PhysicalDevice m_PhysicalDevice;
        vk::Device m_Device;
        vk::DebugUtilsMessengerEXT m_DebugMessenger;

        vk::Queue m_MainQueue;
        uint32 m_MainQueueFamilyIndex;

        vk::CommandPool m_CommandPool;
        vma::Allocator m_Allocator;

        bool m_SupportRaytracing = false;
        bool m_SupportMeshShaders = false;
        uint64 m_OptimalRowPitchAlignment = 0;
        uint64 m_BufferImageGranularity = 0;

        VulkanBindlessManager* m_BindlessManager;
    };
}
