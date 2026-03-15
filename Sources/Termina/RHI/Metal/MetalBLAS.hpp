#pragma once

#include <Termina/RHI/BLAS.hpp>
#include <Metal/Metal.h>

namespace Termina {
    class MetalDevice;

    class MetalBLAS : public BLAS
    {
    public:
        MetalBLAS(MetalDevice* device, const BLASDesc& desc);
        ~MetalBLAS();

        uint64 GetGPUAddress()    const override;
        int32  GetBindlessIndex() const override;

        id<MTLAccelerationStructure> GetAccelerationStructure() const { return mAccelerationStructure; }
    private:
        MetalDevice* m_ParentDevice;
        id<MTLAccelerationStructure> mAccelerationStructure = nil;
    };
}
