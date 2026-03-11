#pragma once

#include <Termina/RHI/Surface.hpp>
#include <Termina/Core/Window.hpp>

#include <vulkan/vulkan.hpp>

#include "VulkanTexture.hpp"
#include "VulkanTextureView.hpp"

namespace Termina {
    class VulkanDevice;

    class VulkanSurface : public RendererSurface
    {
    public:
        VulkanSurface(VulkanDevice* device, Window* window);
        ~VulkanSurface() override;
    
        RenderContext* BeginFrame() override;
        void EndFrame() override;
    
        int GetFrameIndex() const override { return m_CurrentFrame; }
        int GetWidth() const override { return m_Width; }
        int GetHeight() const override { return m_Height; }
        void Resize(int width, int height) override;
    
        RenderContext* GetContext() override { return m_Contexts[m_CurrentFrame]; }
        RendererTexture* GetCurrentTexture() override { return m_Textures[m_ImageIndex]; }
        TextureView* GetCurrentTextureView() override { return m_TextureViews[m_ImageIndex]; }
    private:
        vk::SurfaceKHR m_Surface;
        vk::SwapchainKHR m_Swapchain;
    
        VulkanDevice* m_ParentDevice;
    
        std::vector<VulkanTexture*> m_Textures;
        std::vector<VulkanTextureView*> m_TextureViews;
        std::vector<vk::Semaphore> m_ImageAvailableSemaphores;
        std::vector<vk::Semaphore> m_RenderFinishedSemaphores;
        std::vector<vk::Fence> m_InFlightFences;
        std::vector<RenderContext*> m_Contexts;
        uint32 m_CurrentFrame = 0;
        uint32 m_ImageIndex = 0;
    
        int m_Width = 0;
        int m_Height = 0;
    };
}
