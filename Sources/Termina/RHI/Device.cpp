#include "Vulkan/VulkanDevice.hpp"

namespace Termina {
    RendererDevice* RendererDevice::Create()
    {
        return new VulkanDevice();
    }
}
