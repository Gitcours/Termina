#pragma once

#include <Termina/RHI/Resource.hpp>
#include <Termina/RHI/Texture.hpp>

#include <vulkan/vulkan.hpp>

namespace Termina {
    inline vk::PipelineStageFlags2 ConvertPipelineStageToVulkan(PipelineStage stage)
    {
        vk::PipelineStageFlags2 flags;
    
        if (Any(stage, PipelineStage::TOP_OF_PIPE))
            flags |= vk::PipelineStageFlagBits2::eTopOfPipe;
        if (Any(stage, PipelineStage::DRAW_INDIRECT))
            flags |= vk::PipelineStageFlagBits2::eDrawIndirect;
        if (Any(stage, PipelineStage::VERTEX_INPUT))
            flags |= vk::PipelineStageFlagBits2::eVertexInput;
        if (Any(stage, PipelineStage::INDEX_INPUT))
            flags |= vk::PipelineStageFlagBits2::eVertexInput;
        if (Any(stage, PipelineStage::VERTEX_SHADER))
            flags |= vk::PipelineStageFlagBits2::eVertexShader;
        if (Any(stage, PipelineStage::PIXEL_SHADER))
            flags |= vk::PipelineStageFlagBits2::eFragmentShader;
        if (Any(stage, PipelineStage::COMPUTE_SHADER))
            flags |= vk::PipelineStageFlagBits2::eComputeShader;
        if (Any(stage, PipelineStage::EARLY_FRAGMENT_TESTS))
            flags |= vk::PipelineStageFlagBits2::eEarlyFragmentTests;
        if (Any(stage, PipelineStage::LATE_FRAGMENT_TESTS))
            flags |= vk::PipelineStageFlagBits2::eLateFragmentTests;
        if (Any(stage, PipelineStage::COLOR_ATTACHMENT_OUTPUT))
            flags |= vk::PipelineStageFlagBits2::eColorAttachmentOutput;
        if (Any(stage, PipelineStage::BOTTOM_OF_PIPE))
            flags |= vk::PipelineStageFlagBits2::eBottomOfPipe;
        if (Any(stage, PipelineStage::COPY))
            flags |= vk::PipelineStageFlagBits2::eTransfer;
        if (Any(stage, PipelineStage::ALL_GRAPHICS))
            flags |= vk::PipelineStageFlagBits2::eAllGraphics;
        if (Any(stage, PipelineStage::ALL_COMMANDS))
            flags |= vk::PipelineStageFlagBits2::eAllCommands;
        if (Any(stage, PipelineStage::ACCELERATION_STRUCTURE_WRITE))
            flags |= vk::PipelineStageFlagBits2::eAccelerationStructureBuildKHR;
        if (Any(stage, PipelineStage::ACCELERATION_STRUCTURE_READ))
            flags |= vk::PipelineStageFlagBits2::eAccelerationStructureBuildKHR;
    
        // Vulkan requires srcStageMask to be non-zero. If no stages are set (e.g., NONE),
        // default to TOP_OF_PIPE which is appropriate for initial transitions from UNDEFINED layout.
        if (flags == vk::PipelineStageFlags2{})
            flags = vk::PipelineStageFlagBits2::eTopOfPipe;
    
        return flags;
    }
    
    inline vk::AccessFlags2 ConvertResourceAccessToVulkan(ResourceAccess access)
    {
        vk::AccessFlags2 flags = static_cast<vk::AccessFlagBits2>(0);
        if (Any(access, ResourceAccess::INDIRECT_COMMAND_READ))
            flags |= vk::AccessFlagBits2::eIndirectCommandRead;
        if (Any(access, ResourceAccess::INDEX_READ))
            flags |= vk::AccessFlagBits2::eIndexRead;
        if (Any(access, ResourceAccess::VERTEX_ATTRIBUTE_READ))
            flags |= vk::AccessFlagBits2::eVertexAttributeRead;
        if (Any(access, ResourceAccess::UNIFORM_READ))
            flags |= vk::AccessFlagBits2::eUniformRead;
        if (Any(access, ResourceAccess::SHADER_READ))
            flags |= vk::AccessFlagBits2::eShaderRead;
        if (Any(access, ResourceAccess::SHADER_WRITE))
            flags |= vk::AccessFlagBits2::eShaderWrite;
        if (Any(access, ResourceAccess::COLOR_ATTACHMENT_READ))
            flags |= vk::AccessFlagBits2::eColorAttachmentRead;
        if (Any(access, ResourceAccess::COLOR_ATTACHMENT_WRITE))
            flags |= vk::AccessFlagBits2::eColorAttachmentWrite;
        if (Any(access, ResourceAccess::DEPTH_STENCIL_ATTACHMENT_READ))
            flags |= vk::AccessFlagBits2::eDepthStencilAttachmentRead;
        if (Any(access, ResourceAccess::DEPTH_STENCIL_ATTACHMENT_WRITE))
            flags |= vk::AccessFlagBits2::eDepthStencilAttachmentWrite;
        if (Any(access, ResourceAccess::TRANSFER_READ))
            flags |= vk::AccessFlagBits2::eTransferRead;
        if (Any(access, ResourceAccess::TRANSFER_WRITE))
            flags |= vk::AccessFlagBits2::eTransferWrite;
        if (Any(access, ResourceAccess::HOST_READ))
            flags |= vk::AccessFlagBits2::eHostRead;
        if (Any(access, ResourceAccess::HOST_WRITE))
            flags |= vk::AccessFlagBits2::eHostWrite;
        if (Any(access, ResourceAccess::MEMORY_READ))
            flags |= vk::AccessFlagBits2::eMemoryRead;
        if (Any(access, ResourceAccess::MEMORY_WRITE))
            flags |= vk::AccessFlagBits2::eMemoryWrite;
        if (Any(access, ResourceAccess::ACCELERATION_STRUCTURE_READ))
            flags |= vk::AccessFlagBits2::eAccelerationStructureReadKHR;
        if (Any(access, ResourceAccess::ACCELERATION_STRUCTURE_WRITE))
            flags |= vk::AccessFlagBits2::eAccelerationStructureWriteKHR;
        return flags;
    }
    
    inline vk::ImageLayout ConvertTextureLayoutToVulkan(TextureLayout layout)
    {
        switch (layout)
        {
        case TextureLayout::UNDEFINED:
            return vk::ImageLayout::eUndefined;
        case TextureLayout::GENERAL:
            return vk::ImageLayout::eGeneral;
        case TextureLayout::COLOR_ATTACHMENT:
            return vk::ImageLayout::eColorAttachmentOptimal;
        case TextureLayout::DEPTH_ATTACHMENT:
            return vk::ImageLayout::eDepthStencilAttachmentOptimal;
        case TextureLayout::DEPTH_STENCIL_READ:
            return vk::ImageLayout::eDepthStencilReadOnlyOptimal;
        case TextureLayout::READ_ONLY:
            return vk::ImageLayout::eShaderReadOnlyOptimal;
        case TextureLayout::TRANSFER_SRC:
            return vk::ImageLayout::eTransferSrcOptimal;
        case TextureLayout::TRANSFER_DST:
            return vk::ImageLayout::eTransferDstOptimal;
        case TextureLayout::PRESENT:
            return vk::ImageLayout::ePresentSrcKHR;
        default:
            return vk::ImageLayout::eUndefined;
        }
    }
}
