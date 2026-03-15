#pragma once

#include "BLAS.hpp"
#include "RenderContext.hpp"
#include "Buffer.hpp"

#include <GLM/glm.hpp>
#include <vector>

namespace Termina {
    struct TLASInstanceDesc {
        BLAS*     BLASObject  = nullptr;
        glm::mat4 Transform   = glm::mat4(1.0f); // world transform
        uint32    InstanceID  = 0;
        uint8     Mask        = 0xFF;
        bool      Opaque      = true;
    };

    class TLAS
    {
    public:
        virtual ~TLAS() = default;

        virtual void  Build(RenderContext* ctx,
                            const std::vector<TLASInstanceDesc>& instances,
                            RendererBuffer* scratch,
                            uint64 scratchOffset) = 0;

        virtual int32 GetBindlessIndex() const = 0;
    };
}
