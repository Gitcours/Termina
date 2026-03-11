#pragma once

#include <Termina/Core/Common.hpp>

#include <string>

namespace Termina {
    enum class PipelineStage : uint64
    {
        NONE = BIT(0),
        TOP_OF_PIPE = BIT(1),
        DRAW_INDIRECT = BIT(2),
        VERTEX_INPUT = BIT(3),
        INDEX_INPUT = BIT(4),
        VERTEX_SHADER = BIT(5),
        PIXEL_SHADER = BIT(6),
        COMPUTE_SHADER = BIT(7),
        EARLY_FRAGMENT_TESTS = BIT(8),
        LATE_FRAGMENT_TESTS = BIT(9),
        COLOR_ATTACHMENT_OUTPUT = BIT(10),
        BOTTOM_OF_PIPE = BIT(11),
        COPY = BIT(12),
        ALL_GRAPHICS = BIT(13),
        ALL_COMMANDS = BIT(14),
        ACCELERATION_STRUCTURE_WRITE = BIT(15),
        ACCELERATION_STRUCTURE_READ = BIT(16)
    };
    ENUM_CLASS_FLAG_OPERATORS(PipelineStage);
    
    enum class ResourceAccess : uint64
    {
        NONE = BIT(0),
        INDIRECT_COMMAND_READ = BIT(1),
        INDEX_READ = BIT(2),
        VERTEX_ATTRIBUTE_READ = BIT(3),
        UNIFORM_READ = BIT(4),
        SHADER_READ = BIT(5),
        SHADER_WRITE = BIT(6),
        COLOR_ATTACHMENT_READ = BIT(7),
        COLOR_ATTACHMENT_WRITE = BIT(8),
        DEPTH_STENCIL_ATTACHMENT_READ = BIT(9),
        DEPTH_STENCIL_ATTACHMENT_WRITE = BIT(10),
        TRANSFER_READ = BIT(11),
        TRANSFER_WRITE = BIT(12),
        HOST_READ = BIT(13),
        HOST_WRITE = BIT(14),
        MEMORY_READ = BIT(15),
        MEMORY_WRITE = BIT(16),
        ACCELERATION_STRUCTURE_READ = BIT(17),
        ACCELERATION_STRUCTURE_WRITE = BIT(18)
    };
    ENUM_CLASS_FLAG_OPERATORS(ResourceAccess);
    
    class RendererResource
    {
    public:
        virtual ~RendererResource() = default;
    
        virtual void SetName(const std::string& name) = 0;
    
        ResourceAccess GetLastAccess() const { return m_LastAccess; }
        PipelineStage GetLastPipelineStage() const { return m_LastPipelineStage; }
        void SetLastUsage(ResourceAccess access, PipelineStage stage)
        {
            m_LastAccess = access;
            m_LastPipelineStage = stage;
        }
    
        void SetCurrentAccess(ResourceAccess access) { m_LastAccess = access; }
        void SetCurrentPipelineStage(PipelineStage stage) { m_LastPipelineStage = stage; }
    protected:
        ResourceAccess m_LastAccess = ResourceAccess::NONE;
        PipelineStage m_LastPipelineStage = PipelineStage::NONE;
    };
}
