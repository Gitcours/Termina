#pragma once

#include "Buffer.hpp"

#include <vector>

namespace Termina {
    /// One triangle geometry within a BLAS — corresponds to one MeshInstance.
    struct BLASGeometry {
        uint32 VertexOffset = 0; // BaseVertex (in vertex units)
        uint32 VertexCount  = 0;
        uint32 IndexOffset  = 0; // in index units (LODs[0].IndexOffset)
        uint32 IndexCount   = 0;
        bool   Opaque       = true;
    };

    struct BLASDesc {
        RendererBuffer*            VertexBuffer = nullptr;
        RendererBuffer*            IndexBuffer  = nullptr;
        std::vector<BLASGeometry>  Geometries;
    };

    class BLAS
    {
    public:
        virtual ~BLAS() = default;

        const BLASDesc& GetDesc() const { return m_Desc; }

        virtual uint64 GetGPUAddress()    const = 0;
        virtual int32  GetBindlessIndex() const = 0;
    protected:
        BLASDesc m_Desc;
    };
}
