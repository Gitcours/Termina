#pragma once

#include <string>

#include <Termina/Core/Common.hpp>

namespace Termina {
    enum class PipelineType
    {
        Graphics,
        Compute,
        Mesh
    };
    
    class Pipeline
    {
    public:
        virtual ~Pipeline() = default;
    
        virtual PipelineType GetType() const = 0;
        virtual uint64 GetSize() const = 0;
    };
}
