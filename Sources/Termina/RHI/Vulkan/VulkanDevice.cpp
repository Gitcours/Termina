#include "VulkanDevice.hpp"
#include "VulkanSurface.hpp"
#include "VulkanRenderContext.hpp"
#include "VulkanTexture.hpp"
#include "VulkanTextureView.hpp"
#include "VulkanRenderPipeline.hpp"
#include "VulkanBuffer.hpp"
#include "VulkanBufferView.hpp"
#include "VulkanSampler.hpp"
#include "VulkanComputePipeline.hpp"
#include "VulkanBLAS.hpp"
#include "VulkanTLAS.hpp"

#include <Termina/Core/Logger.hpp>

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>

#include <map>
#include <vector>

#include <GLFW/glfw3.h>

// Define the default dispatcher dynamic loader for Vulkan-Hpp
VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

namespace Termina {
    static vk::detail::DynamicLoader dl;

    static VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugCallback(
        vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        vk::DebugUtilsMessageTypeFlagsEXT messageType,
        const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData)
    {
        (void)messageType;
        (void)pUserData;
    
        if (!strcmp(pCallbackData->pMessageIdName, "VUID-VkShaderModuleCreateInfo-pCode-08737")) return VK_FALSE;
        switch (messageSeverity)
        {
            case vk::DebugUtilsMessageSeverityFlagBitsEXT::eError: {
                TN_ERROR("Vulkan Error: %s", pCallbackData->pMessage);
                break;
            }
            default:
                break;
        }
        return VK_FALSE;
    }
    
    VulkanDevice::VulkanDevice()
    {
        CreateInstance();
        CreatePhysicalDevice();
        CreateDevice();
        CreateCommandPool();
        CreateAllocator();
    
        m_BindlessManager = new VulkanBindlessManager(this);
    }
    
    VulkanDevice::~VulkanDevice()
    {
        delete m_BindlessManager;
        m_Allocator.destroy();
        m_Device.destroyCommandPool(m_CommandPool);
        m_Device.destroy();
        if (m_DebugMessenger) m_Instance.destroyDebugUtilsMessengerEXT(m_DebugMessenger, nullptr);
        m_Instance.destroy();
    }
    
    void VulkanDevice::ExecuteRenderContext(RenderContext* context)
    {
        VulkanRenderContext* vkContext = static_cast<VulkanRenderContext*>(context);
    
        vk::SubmitInfo submitInfo{};
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &vkContext->GetCommandBuffer();
    
        m_MainQueue.submit(submitInfo, nullptr);
    }
    
    void VulkanDevice::WaitIdle()
    {
        m_Device.waitIdle();
    }
    
    void VulkanDevice::CreateInstance()
    {
        // Initialize the dynamic dispatcher loader (must be static to persist)
        PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr =
            dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
        VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);
    
        vk::ApplicationInfo appInfo;
        appInfo.setApplicationVersion(VK_MAKE_VERSION(1, 0, 0))
               .setEngineVersion(VK_MAKE_VERSION(1, 0, 0))
               .setApiVersion(VK_API_VERSION_1_4)
               .setPApplicationName("GameEngine")
               .setPEngineName("GameEngine");
        
        vk::DebugUtilsMessengerCreateInfoEXT messengerInfo = {};
        messengerInfo.messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eError |
                                        vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
                                        vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose;
        messengerInfo.messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                                    vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
                                    vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance;
        messengerInfo.pfnUserCallback = VulkanDebugCallback;
        messengerInfo.pUserData = nullptr;
    
        std::vector<const char*> instanceExtensions = {
            VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
            VK_KHR_SURFACE_EXTENSION_NAME
        };
        if (glfwVulkanSupported()) {
            uint32_t glfwExtensionCount = 0;
            const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
            for (uint32 i = 0; i < glfwExtensionCount; i++) {
                instanceExtensions.push_back(glfwExtensions[i]);
            }
        }
        std::vector<const char*> instanceLayers = { "VK_LAYER_KHRONOS_validation" };
    
        vk::InstanceCreateInfo instanceCreateInfo;
        instanceCreateInfo.setPApplicationInfo(&appInfo);
        instanceCreateInfo.setEnabledExtensionCount(static_cast<uint32_t>(instanceExtensions.size()));
        instanceCreateInfo.setPpEnabledExtensionNames(instanceExtensions.data());
        //instanceCreateInfo.setEnabledLayerCount(static_cast<uint32_t>(instanceLayers.size()));
        //instanceCreateInfo.setPpEnabledLayerNames(instanceLayers.data());
    
        m_Instance = vk::createInstance(instanceCreateInfo, nullptr);
    
        // Initialize instance-specific function pointers
        VULKAN_HPP_DEFAULT_DISPATCHER.init(m_Instance);
    
    #if _DEBUG
        vk::DebugUtilsMessengerCreateInfoEXT debugCreateInfo = messengerInfo;
        m_DebugMessenger = m_Instance.createDebugUtilsMessengerEXT(debugCreateInfo, nullptr);
    #endif
    }
    
    void VulkanDevice::CreatePhysicalDevice()
    {
        auto physicalDevices = m_Instance.enumeratePhysicalDevices();
        for (const auto& device : physicalDevices) {
            vk::PhysicalDeviceProperties properties = device.getProperties();
            if (properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu) {
                m_PhysicalDevice = device;
                break;
            }
        }
    
        auto properties = m_PhysicalDevice.getProperties();
        TN_INFO("Using Vulkan device: %s", properties.deviceName.data());
    
        m_OptimalRowPitchAlignment = properties.limits.optimalBufferCopyRowPitchAlignment;
        m_BufferImageGranularity = properties.limits.bufferImageGranularity;
    }
    
    void VulkanDevice::CreateDevice()
    {
        std::vector<const char*> mandatoryExtensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
            VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
            VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
            VK_EXT_MUTABLE_DESCRIPTOR_TYPE_EXTENSION_NAME,
            VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
            VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME
        };
    
        std::map<const char*, bool> optionalExtensions =  {
            {VK_EXT_MESH_SHADER_EXTENSION_NAME, false},
            {VK_KHR_RAY_QUERY_EXTENSION_NAME, false},
            {VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME, false},
            {VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME, false}
        };
    
        auto availableExtensions = m_PhysicalDevice.enumerateDeviceExtensionProperties();
        for (const auto& availableExtension : availableExtensions) {
            for (auto& extension : optionalExtensions) {
                if (strcmp(availableExtension.extensionName, extension.first) == 0) {
                    extension.second = true;
                    break;
                }
            }
        }
    
        for (const auto& extension : optionalExtensions) {
            if (extension.second) {
                mandatoryExtensions.push_back(extension.first);
                TN_DEBUG("Enabling optional Vulkan extension: %s", extension.first);
            }
        }
    
        vk::PhysicalDeviceVulkan11Features vulkan11Features;
        vulkan11Features.setShaderDrawParameters(true);
    
        vk::PhysicalDeviceVulkan12Features vulkan12Features;
        vulkan12Features.setBufferDeviceAddress(true);
        vulkan12Features.setDescriptorIndexing(true);
        vulkan12Features.setRuntimeDescriptorArray(true);
        vulkan12Features.setShaderSampledImageArrayNonUniformIndexing(true);
        vulkan12Features.setDescriptorBindingVariableDescriptorCount(true);
        vulkan12Features.setDescriptorBindingPartiallyBound(true);
        vulkan12Features.setDescriptorBindingSampledImageUpdateAfterBind(true);
        vulkan12Features.setPNext(&vulkan11Features);
    
        vk::PhysicalDeviceVulkan13Features vulkan13Features;
        vulkan13Features.setDynamicRendering(true);
        vulkan13Features.setSynchronization2(true);
        vulkan13Features.setPNext(&vulkan12Features);
    
        vk::PhysicalDeviceMutableDescriptorTypeFeaturesEXT mutableDescriptorTypeFeatures;
        mutableDescriptorTypeFeatures.setMutableDescriptorType(true);
        mutableDescriptorTypeFeatures.setPNext(&vulkan13Features);
    
        void* lastPNext = (void*)&mutableDescriptorTypeFeatures;
    
        vk::PhysicalDeviceMeshShaderFeaturesEXT meshShaderFeatures;
        vk::PhysicalDeviceRayQueryFeaturesKHR rayQueryFeatures;
        vk::PhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureFeatures;
    
        if (optionalExtensions[VK_EXT_MESH_SHADER_EXTENSION_NAME]) {
            meshShaderFeatures.setMeshShader(true);
            meshShaderFeatures.setTaskShader(true);
            meshShaderFeatures.setPNext(lastPNext);
            lastPNext = &meshShaderFeatures;
        
            m_SupportMeshShaders = true;
        }

        if (optionalExtensions[VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME]) {
            accelerationStructureFeatures.setAccelerationStructure(true);
            accelerationStructureFeatures.setDescriptorBindingAccelerationStructureUpdateAfterBind(true);
            accelerationStructureFeatures.setPNext(lastPNext);
            lastPNext = &accelerationStructureFeatures;   
        }
    
        if (optionalExtensions[VK_KHR_RAY_QUERY_EXTENSION_NAME]) {
            rayQueryFeatures.setRayQuery(true);
            rayQueryFeatures.setPNext(lastPNext);
            lastPNext = &rayQueryFeatures;

            // Raytracing only supported if both ray query and acceleration structure features are available
            m_SupportRaytracing = true;
        }
    
        vk::PhysicalDeviceFeatures2 deviceFeatures;
        deviceFeatures.features.setSamplerAnisotropy(true);
        deviceFeatures.features.setDepthClamp(true);
        deviceFeatures.features.setFillModeNonSolid(true);
        deviceFeatures.features.setMultiDrawIndirect(true);
        deviceFeatures.features.setTextureCompressionBC(true);
        deviceFeatures.features.setDrawIndirectFirstInstance(true);
        deviceFeatures.setPNext(lastPNext);
    
        // Queues
        auto queues = m_PhysicalDevice.getQueueFamilyProperties();
        for (uint32 i = 0; i < static_cast<uint32>(queues.size()); i++) {
            if (queues[i].queueFlags & vk::QueueFlagBits::eGraphics) {
                m_MainQueueFamilyIndex = i;
                break;
            }
        }
    
        float queuePriority = 1.0f;
        vk::DeviceQueueCreateInfo queueCreateInfo;
        queueCreateInfo.setQueueFamilyIndex(m_MainQueueFamilyIndex);
        queueCreateInfo.setQueueCount(1);
        queueCreateInfo.setPQueuePriorities(&queuePriority);
    
        vk::DeviceCreateInfo deviceCreateInfo;
        deviceCreateInfo.setPNext(&deviceFeatures);
        deviceCreateInfo.setQueueCreateInfoCount(1);
        deviceCreateInfo.setPQueueCreateInfos(&queueCreateInfo);
        deviceCreateInfo.setEnabledExtensionCount(static_cast<uint32_t>(mandatoryExtensions.size()));
        deviceCreateInfo.setPpEnabledExtensionNames(mandatoryExtensions.data());
    
        m_Device = m_PhysicalDevice.createDevice(deviceCreateInfo, nullptr);
        m_MainQueue = m_Device.getQueue(m_MainQueueFamilyIndex, 0);
    
        VULKAN_HPP_DEFAULT_DISPATCHER.init(m_Device);
    }
    
    void VulkanDevice::CreateCommandPool()
    {
        vk::CommandPoolCreateInfo poolCreateInfo;
        poolCreateInfo.setQueueFamilyIndex(m_MainQueueFamilyIndex);
        poolCreateInfo.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
    
        m_CommandPool = m_Device.createCommandPool(poolCreateInfo);
    }
    
    void VulkanDevice::CreateAllocator()
    {
        vma::VulkanFunctions vulkanFunctions = {};
        vulkanFunctions.vkGetInstanceProcAddr = dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
        vulkanFunctions.vkGetDeviceProcAddr = dl.getProcAddress<PFN_vkGetDeviceProcAddr>("vkGetDeviceProcAddr");
    
        vma::AllocatorCreateInfo allocatorInfo = {};
        allocatorInfo.physicalDevice = m_PhysicalDevice;
        allocatorInfo.device = m_Device;
        allocatorInfo.instance = m_Instance;
        allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_4;
        allocatorInfo.flags = vma::AllocatorCreateFlagBits::eBufferDeviceAddress;
        allocatorInfo.pVulkanFunctions = &vulkanFunctions;
    
        vk::Result result =  vma::createAllocator(&allocatorInfo, &m_Allocator);
        if (result != vk::Result::eSuccess) {
            TN_ERROR("Failed to create Vulkan Memory Allocator!");
        }
    }
    
    RendererSurface* VulkanDevice::CreateSurface(Window* window)
    {
        return new VulkanSurface(this, window);
    }
    
    RenderContext* VulkanDevice::CreateRenderContext(bool singleTime)
    {
        return new VulkanRenderContext(this, singleTime);
    }
    
    RendererTexture* VulkanDevice::CreateTexture(const TextureDesc& desc)
    {
        return new VulkanTexture(desc, m_Device, m_Allocator);
    }
    
    TextureView* VulkanDevice::CreateTextureView(const TextureViewDesc& desc)
    {
        return new VulkanTextureView(this, desc);
    }
    
    RenderPipeline* VulkanDevice::CreateRenderPipeline(const RenderPipelineDesc& desc)
    {
        return new VulkanRenderPipeline(this, desc);
    }
    
    RendererBuffer* VulkanDevice::CreateBuffer(const BufferDesc& desc)
    {
        return new VulkanBuffer(this, desc);
    }
    
    BufferView* VulkanDevice::CreateBufferView(const BufferViewDesc& desc)
    {
        return new VulkanBufferView(this, desc);
    }
    
    Sampler* VulkanDevice::CreateSampler(const SamplerDesc& desc)
    {
        return new VulkanSampler(this, desc);
    }
    
    ComputePipeline* VulkanDevice::CreateComputePipeline(const ShaderModule& module, const std::string& name)
    {
        return new VulkanComputePipeline(this, module, name);
    }

    BLAS* VulkanDevice::CreateBLAS(const BLASDesc& desc)
    {
        if (!m_SupportRaytracing) return nullptr;
        return new VulkanBLAS(this, desc);
    }

    TLAS* VulkanDevice::CreateTLAS(uint32 maxInstances)
    {
        if (!m_SupportRaytracing) return nullptr;
        return new VulkanTLAS(this, maxInstances);
    }
}
