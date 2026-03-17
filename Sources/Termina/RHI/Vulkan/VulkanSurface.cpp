#include "VulkanSurface.hpp"
#include "VulkanDevice.hpp"
#include "VulkanRenderContext.hpp"
#include "vulkan/vulkan_core.h"
#include "vulkan/vulkan_to_string.hpp"

#include <Termina/Core/Logger.hpp>
#include <Termina/Core/Assert.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace Termina {
    VulkanSurface::VulkanSurface(VulkanDevice* device, Window* window)
        : m_ParentDevice(device)
    {
        // Surface - Use GLFW to create the Vulkan surface
        VkSurfaceKHR surface = VK_NULL_HANDLE;
        VkResult result = glfwCreateWindowSurface(device->GetVulkanInstance(), window->GetHandle(), NULL, &surface);
        TN_ASSERT(result == VK_SUCCESS, "Failed to create surface!");

        m_Surface = vk::SurfaceKHR(surface);

        // Swapchain
        vk::SwapchainCreateInfoKHR swapchainCreateInfo;
        swapchainCreateInfo.setSurface(m_Surface);
        swapchainCreateInfo.setMinImageCount(2);
        swapchainCreateInfo.setImageFormat(vk::Format::eB8G8R8A8Unorm);
        swapchainCreateInfo.setImageColorSpace(vk::ColorSpaceKHR::eSrgbNonlinear);
        swapchainCreateInfo.setImageExtent(vk::Extent2D{ static_cast<uint32_t>(window->GetWidth()), static_cast<uint32_t>(window->GetHeight()) });
        swapchainCreateInfo.setImageArrayLayers(1);
        swapchainCreateInfo.setImageUsage(vk::ImageUsageFlagBits::eColorAttachment);
        swapchainCreateInfo.setImageSharingMode(vk::SharingMode::eExclusive);
        swapchainCreateInfo.setPreTransform(vk::SurfaceTransformFlagBitsKHR::eIdentity);
        swapchainCreateInfo.setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque);
        swapchainCreateInfo.setPresentMode(vk::PresentModeKHR::eMailbox);
        swapchainCreateInfo.setClipped(VK_TRUE);

        m_Swapchain = device->GetVulkanDevice().createSwapchainKHR(swapchainCreateInfo);

        auto swapchainImages = device->GetVulkanDevice().getSwapchainImagesKHR(m_Swapchain);
        for (uint64 i = 0; i < swapchainImages.size(); i++) {
            VulkanTexture* texture = new VulkanTexture();
            texture->m_Image = swapchainImages[i];
            texture->m_Desc.Width = window->GetWidth();
            texture->m_Desc.Height = window->GetHeight();
            texture->m_Desc.Format = TextureFormat::BGRA8_UNORM;
            texture->m_Desc.Usage = TextureUsage::RENDER_TARGET;
            texture->m_ParentDevice = m_ParentDevice->GetVulkanDevice();
            texture->m_ShouldDestroy = false;
            texture->SetName("Swapchain Texture " + std::to_string(i));
            texture->SetCurrentLayout(TextureLayout::UNDEFINED);

            m_Textures.push_back(texture);
        }

        m_Width = window->GetWidth();
        m_Height = window->GetHeight();

        // Create synchronization primitives
        uint32 imageCount = FRAMES_IN_FLIGHT;
        m_ImageAvailableSemaphores.resize(imageCount);
        m_RenderFinishedSemaphores.resize(imageCount);
        m_InFlightFences.resize(imageCount);
        m_Contexts.resize(imageCount);
        m_TextureViews.resize(m_Textures.size());

        for (uint32 i = 0; i < imageCount; i++) {
            vk::SemaphoreCreateInfo semaphoreInfo;
            m_ImageAvailableSemaphores[i] = device->GetVulkanDevice().createSemaphore(semaphoreInfo);
            m_RenderFinishedSemaphores[i] = device->GetVulkanDevice().createSemaphore(semaphoreInfo);

            vk::FenceCreateInfo fenceInfo;
            fenceInfo.setFlags(vk::FenceCreateFlagBits::eSignaled);
            m_InFlightFences[i] = device->GetVulkanDevice().createFence(fenceInfo);

            m_Contexts[i] = new VulkanRenderContext(device, false);
        }

        // Create texture views for ALL swapchain images (not just imageCount)
        for (uint32 i = 0; i < m_Textures.size(); i++) {
            TextureViewDesc viewDesc = TextureViewDesc::CreateDefault(m_Textures[i], TextureViewType::RENDER_TARGET, TextureViewDimension::TEXTURE_2D);
            m_TextureViews[i] = reinterpret_cast<VulkanTextureView*>(device->CreateTextureView(viewDesc));
        }
    }

    VulkanSurface::~VulkanSurface()
    {
        // Clean up textures and texture views (one per swapchain image)
        for (uint64 i = 0; i < m_Textures.size(); i++) {
            delete m_Textures[i];
            delete m_TextureViews[i];
        }

        // Clean up synchronization primitives and contexts (one per frame in flight)
        for (uint64 i = 0; i < m_ImageAvailableSemaphores.size(); i++) {
            delete m_Contexts[i];
            m_ParentDevice->GetVulkanDevice().destroySemaphore(m_ImageAvailableSemaphores[i]);
            m_ParentDevice->GetVulkanDevice().destroySemaphore(m_RenderFinishedSemaphores[i]);
            m_ParentDevice->GetVulkanDevice().destroyFence(m_InFlightFences[i]);
        }

        m_ParentDevice->GetVulkanDevice().destroySwapchainKHR(m_Swapchain);
        m_ParentDevice->GetVulkanInstance().destroySurfaceKHR(m_Surface);
    }

    RenderContext* VulkanSurface::BeginFrame()
    {
        // Wait for previous frame to complete
        vk::Result result = m_ParentDevice->GetVulkanDevice().waitForFences(1, &m_InFlightFences[m_CurrentFrame], VK_TRUE, UINT64_MAX);
        if (result == vk::Result::eTimeout) {
            TN_ERROR("vkWaitForFences timed out!");
            return nullptr;
        }

        auto acquireResult = m_ParentDevice->GetVulkanDevice().acquireNextImageKHR(
            m_Swapchain,
            UINT64_MAX,
            m_ImageAvailableSemaphores[m_CurrentFrame],
            nullptr
        );

        if (acquireResult.result != vk::Result::eSuccess) {
            if (acquireResult.result == vk::Result::eErrorOutOfDateKHR) {
                TN_WARN("Swapchain out of date during acquire, needs resize");
                // Advance to next frame to avoid potentially corrupted semaphore
                m_CurrentFrame = (m_CurrentFrame + 1) % FRAMES_IN_FLIGHT;
                return nullptr;
            } else if (acquireResult.result == vk::Result::eErrorSurfaceLostKHR) {
                TN_ERROR("Surface lost!");
                // Advance to next frame to avoid potentially corrupted semaphore
                m_CurrentFrame = (m_CurrentFrame + 1) % FRAMES_IN_FLIGHT;
                return nullptr;
            } else {
                TN_ERROR("Failed to acquire next image: {}", static_cast<int>(acquireResult.result));
                return nullptr;
            }
        }

        m_ImageIndex = acquireResult.value;

        // Reset fence after successful acquire
        result = m_ParentDevice->GetVulkanDevice().resetFences(1, &m_InFlightFences[m_CurrentFrame]);
        if (result != vk::Result::eSuccess) {
            TN_ERROR("Failed to reset fences!");
            return nullptr;
        }

        auto context = m_Contexts[m_CurrentFrame];
        context->Reset();
        context->Begin();
        return m_Contexts[m_CurrentFrame];
    }

    void VulkanSurface::EndFrame()
    {
        // Switch to present
        {
            vk::CommandBuffer commandBuffer = reinterpret_cast<VulkanRenderContext*>(m_Contexts[m_CurrentFrame])->GetCommandBuffer();

            vk::ImageMemoryBarrier barrier;
            barrier.setSrcAccessMask(vk::AccessFlagBits::eColorAttachmentWrite);
            barrier.setDstAccessMask(vk::AccessFlagBits::eMemoryRead);
            barrier.setOldLayout(vk::ImageLayout::eColorAttachmentOptimal);
            barrier.setNewLayout(vk::ImageLayout::ePresentSrcKHR);
            barrier.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
            barrier.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
            barrier.setImage(m_Textures[m_ImageIndex]->m_Image);
            barrier.subresourceRange.setAspectMask(vk::ImageAspectFlagBits::eColor);
            barrier.subresourceRange.setBaseMipLevel(0);
            barrier.subresourceRange.setLevelCount(1);
            barrier.subresourceRange.setBaseArrayLayer(0);
            barrier.subresourceRange.setLayerCount(1);

            commandBuffer.pipelineBarrier(
                vk::PipelineStageFlagBits::eColorAttachmentOutput,
                vk::PipelineStageFlagBits::eBottomOfPipe,
                vk::DependencyFlags(),
                0, nullptr,
                0, nullptr,
                1, &barrier
            );

            // Update tracked layout state
            m_Textures[m_ImageIndex]->SetCurrentLayout(TextureLayout::PRESENT);

            commandBuffer.end();
        }

        // Submit
        {
            vk::CommandBuffer commandBuffer = reinterpret_cast<VulkanRenderContext*>(m_Contexts[m_CurrentFrame])->GetCommandBuffer();
            vk::Semaphore waitSemaphores[] = { m_ImageAvailableSemaphores[m_CurrentFrame] };
            vk::PipelineStageFlags waitStages[] = { vk::PipelineStageFlagBits::eAllCommands };
            vk::Semaphore signalSemaphores[] = { m_RenderFinishedSemaphores[m_CurrentFrame] };

            vk::SubmitInfo submitInfo;
            submitInfo.setWaitSemaphoreCount(1);
            submitInfo.setPWaitSemaphores(waitSemaphores);
            submitInfo.setPWaitDstStageMask(waitStages);
            submitInfo.setCommandBufferCount(1);
            submitInfo.setPCommandBuffers(&commandBuffer);
            submitInfo.setSignalSemaphoreCount(1);
            submitInfo.setPSignalSemaphores(signalSemaphores);

            vk::Result result = m_ParentDevice->GetMainQueue().submit(1, &submitInfo, m_InFlightFences[m_CurrentFrame]);
            if (result != vk::Result::eSuccess) {
                TN_ERROR("Failed to submit draw command buffer: %s", vk::to_string(result).c_str());
            }
        }

        // Present
        {
            vk::Semaphore waitSemaphores[] = { m_RenderFinishedSemaphores[m_CurrentFrame] };
            vk::SwapchainKHR swapchains[] = { m_Swapchain };

            vk::PresentInfoKHR presentInfo;
            presentInfo.setWaitSemaphoreCount(1);
            presentInfo.setPWaitSemaphores(waitSemaphores);
            presentInfo.setSwapchainCount(1);
            presentInfo.setPSwapchains(swapchains);
            presentInfo.setPImageIndices(&m_ImageIndex);

            vk::Result result = m_ParentDevice->GetMainQueue().presentKHR(presentInfo);
            if (result == vk::Result::eErrorOutOfDateKHR) {
                TN_WARN("Swapchain out of date during present, needs resize");
            } else if (result == vk::Result::eErrorSurfaceLostKHR) {
                TN_ERROR("Surface lost!");
            } else if (result != vk::Result::eSuccess) {
                TN_ERROR("Failed to present swapchain image!");
            }

            m_CurrentFrame = (m_CurrentFrame + 1) % FRAMES_IN_FLIGHT;
        }
    }

    void VulkanSurface::Resize(int width, int height)
    {
        m_ParentDevice->WaitIdle();

        m_Width = width;
        m_Height = height;

        // Reset frame counter to start fresh after resize
        m_CurrentFrame = 0;

        // Clean up synchronization primitives first - they may be in undefined state after failed acquire
        uint32 imageCount = FRAMES_IN_FLIGHT;
        for (uint32 i = 0; i < imageCount; i++) {
            m_ParentDevice->GetVulkanDevice().destroySemaphore(m_ImageAvailableSemaphores[i]);
            m_ParentDevice->GetVulkanDevice().destroySemaphore(m_RenderFinishedSemaphores[i]);
            m_ParentDevice->GetVulkanDevice().destroyFence(m_InFlightFences[i]);
        }

        // Recreate synchronization primitives in clean state
        for (uint32 i = 0; i < imageCount; i++) {
            vk::SemaphoreCreateInfo semaphoreInfo;
            m_ImageAvailableSemaphores[i] = m_ParentDevice->GetVulkanDevice().createSemaphore(semaphoreInfo);
            m_RenderFinishedSemaphores[i] = m_ParentDevice->GetVulkanDevice().createSemaphore(semaphoreInfo);

            vk::FenceCreateInfo fenceInfo;
            fenceInfo.setFlags(vk::FenceCreateFlagBits::eSignaled);
            m_InFlightFences[i] = m_ParentDevice->GetVulkanDevice().createFence(fenceInfo);
        }

        // Clean up ALL old textures and texture views
        for (uint64 i = 0; i < m_Textures.size(); i++) {
            delete m_Textures[i];
            delete m_TextureViews[i];
        }
        m_Textures.clear();
        m_ParentDevice->GetVulkanDevice().destroySwapchainKHR(m_Swapchain);

        vk::SwapchainCreateInfoKHR swapchainCreateInfo;
        swapchainCreateInfo.setSurface(m_Surface);
        swapchainCreateInfo.setMinImageCount(2);
        swapchainCreateInfo.setImageFormat(vk::Format::eB8G8R8A8Unorm);
        swapchainCreateInfo.setImageColorSpace(vk::ColorSpaceKHR::eSrgbNonlinear);
        swapchainCreateInfo.setImageExtent(vk::Extent2D{ static_cast<uint32_t>(m_Width), static_cast<uint32_t>(m_Height) });
        swapchainCreateInfo.setImageArrayLayers(1);
        swapchainCreateInfo.setImageUsage(vk::ImageUsageFlagBits::eColorAttachment);
        swapchainCreateInfo.setImageSharingMode(vk::SharingMode::eExclusive);
        swapchainCreateInfo.setPreTransform(vk::SurfaceTransformFlagBitsKHR::eIdentity);
        swapchainCreateInfo.setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque);
        swapchainCreateInfo.setPresentMode(vk::PresentModeKHR::eMailbox);
        swapchainCreateInfo.setClipped(VK_TRUE);

        m_Swapchain = m_ParentDevice->GetVulkanDevice().createSwapchainKHR(swapchainCreateInfo);

        auto swapchainImages = m_ParentDevice->GetVulkanDevice().getSwapchainImagesKHR(m_Swapchain);

        // Resize vectors to match new swapchain image count
        m_Textures.resize(swapchainImages.size());
        m_TextureViews.resize(swapchainImages.size());

        // Create new textures for all swapchain images
        for (uint64 i = 0; i < swapchainImages.size(); i++) {
            VulkanTexture* texture = new VulkanTexture();
            texture->m_Image = swapchainImages[i];
            texture->m_Desc.Width = m_Width;
            texture->m_Desc.Height = m_Height;
            texture->m_Desc.Format = TextureFormat::BGRA8_UNORM;
            texture->m_Desc.Usage = TextureUsage::RENDER_TARGET;
            texture->m_ParentDevice = m_ParentDevice->GetVulkanDevice();
            texture->m_ShouldDestroy = false;
            texture->SetName("Swapchain Texture " + std::to_string(i));
            // Swapchain images start in UNDEFINED layout
            texture->SetCurrentLayout(TextureLayout::UNDEFINED);

            m_Textures[i] = texture;
        }

        // Create texture views for ALL swapchain images
        for (uint32 i = 0; i < swapchainImages.size(); i++) {
            TextureViewDesc viewDesc = TextureViewDesc::CreateDefault(m_Textures[i], TextureViewType::RENDER_TARGET, TextureViewDimension::TEXTURE_2D);
            m_TextureViews[i] = reinterpret_cast<VulkanTextureView*>(m_ParentDevice->CreateTextureView(viewDesc));
        }
    }
}
