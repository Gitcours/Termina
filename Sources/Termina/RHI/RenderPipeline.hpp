#pragma once

#include <Termina/Core/Common.hpp>

#include "Texture.hpp"
#include "Pipeline.hpp"

#include <Termina/Shader/ShaderCompiler.h>
#include <Termina/Shader/ShaderTypes.h>

#include <unordered_map>
#include <vector>

namespace Termina {
    enum class PipelineTopology
    {
        TRIANGLES,
        LINES,
        POINTS
    };
    
    enum class PipelineFillMode
    {
        SOLID,
        WIREFRAME
    };
    
    enum class PipelineCullMode
    {
        NONE,
        FRONT,
        BACK
    };
    
    enum class PipelineCompareOp
    {
        NEVER,
        LESS,
        EQUAL,
        LESS_EQUAL,
        GREATER,
        NOT_EQUAL,
        GREATER_EQUAL,
        ALWAYS
    };
    
    struct RenderPipelineDesc
    {
        PipelineFillMode FillMode = PipelineFillMode::SOLID;
        PipelineCullMode CullMode = PipelineCullMode::BACK;
        PipelineTopology Topology = PipelineTopology::TRIANGLES;
        PipelineCompareOp DepthCompareOp = PipelineCompareOp::LESS;
        bool CCW = true;
    
        bool DepthReadEnabled = false;
        bool DepthWriteEnabled = false;
        bool DepthClampEnabled = false;
        bool EnableBlending = false;
    
        std::vector<TextureFormat> ColorAttachmentFormats;
        TextureFormat DepthAttachmentFormat = TextureFormat::UNDEFINED;
    
        std::unordered_map<ShaderType, ShaderModule> ShaderModules;
        uint64 PushConstantSize = 0;
    
        std::string Name = "Render Pipeline";
    
        RenderPipelineDesc& SetName(const std::string& name)
        {
            Name = name;
            return *this;
        }
    
        RenderPipelineDesc& SetFillMode(PipelineFillMode fillMode)
        {
            FillMode = fillMode;
            return *this;
        }
    
        RenderPipelineDesc& SetCullMode(PipelineCullMode cullMode)
        {
            CullMode = cullMode;
            return *this;
        }
    
        RenderPipelineDesc& SetTopology(PipelineTopology topology)
        {
            Topology = topology;
            return *this;
        }
    
        RenderPipelineDesc& SetDepthCompareOp(PipelineCompareOp compareOp)
        {
            DepthCompareOp = compareOp;
            return *this;
        }
    
        RenderPipelineDesc& SetDepthReadEnabled(bool enabled)
        {
            DepthReadEnabled = enabled;
            return *this;
        }
    
        RenderPipelineDesc& SetDepthWriteEnabled(bool enabled)
        {
            DepthWriteEnabled = enabled;
            return *this;
        }
    
        RenderPipelineDesc& SetDepthClampEnabled(bool enabled)
        {
            DepthClampEnabled = enabled;
            return *this;
        }
    
        RenderPipelineDesc& SetEnableBlending(bool enabled)
        {
            EnableBlending = enabled;
            return *this;
        }
    
        RenderPipelineDesc& AddColorAttachmentFormat(TextureFormat format)
        {
            ColorAttachmentFormats.push_back(format);
            return *this;
        }
    
        RenderPipelineDesc& SetDepthAttachmentFormat(TextureFormat format)
        {
            DepthAttachmentFormat = format;
            return *this;
        }
    
        RenderPipelineDesc& SetShaderBytecode(ShaderType type, const std::vector<uint8>& bytecode, const std::string& entryPoint)
        {
            ShaderModules[type] = { type, entryPoint, bytecode };
            return *this;
        }
    
        RenderPipelineDesc& SetPushConstantSize(uint64 size)
        {
            PushConstantSize = size;
            return *this;
        }
    };
    
    class RenderPipeline : public Pipeline
    {
    public:
        virtual ~RenderPipeline() = default;
    
        const RenderPipelineDesc& GetDesc() const { return m_Desc; }
        PipelineType GetType() const override { return PipelineType::Graphics; }
    
        std::string GetName() const { return m_Desc.Name; }
    protected:
        RenderPipelineDesc m_Desc;
    };
}
