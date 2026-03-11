#pragma once

#include <Termina/Core/Common.hpp>
#include <Termina/Shader/ShaderCompiler.h>
#include <Termina/Shader/ShaderTypes.h>

#include "Pipeline.hpp"

namespace Termina {
    class ComputePipeline : public Pipeline
    {
    public:
        virtual ~ComputePipeline() = default;
    
        PipelineType GetType() const override { return PipelineType::Compute; }
    
        std::string GetName() const { return m_Name; }
    protected:
        std::string m_Name;
    };
}
