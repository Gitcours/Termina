#pragma once

#include "Pipeline.hpp"
#include "Buffer.hpp"

#include <Termina/Core/Common.hpp>

namespace Termina {
    class ComputeEncoder
    {
    public:
        virtual ~ComputeEncoder() = default;
    
        virtual void SetPipeline(Pipeline* pipeline) = 0;
        virtual void SetConstants(uint32 size, const void* data) = 0;
    
        virtual void Dispatch(uint32 x, uint32 y, uint32 z, uint32 groupSizeX = 1, uint32 groupSizeY = 1, uint32 groupSizeZ = 1) = 0;
        virtual void End() = 0;
    };
}
