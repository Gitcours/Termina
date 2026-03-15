#pragma once

#include <Termina/RHI/TLAS.hpp>
#include <Metal/Metal.h>

namespace Termina {
    class MetalDevice;
    class MetalBuffer;

    class MetalTLAS : public TLAS
    {
    public:
        MetalTLAS(MetalDevice* device, uint32 maxInstances);
        ~MetalTLAS() override;

        void  Build(RenderContext* ctx,
                    const std::vector<TLASInstanceDesc>& instances,
                    RendererBuffer* scratch,
                    uint64 scratchOffset) override;

        int32 GetBindlessIndex() const override { return m_BindlessIndex; }

    private:
        MetalDevice*                      m_Device;
        uint32                            m_MaxInstances;

        id<MTLAccelerationStructure> mAccelerationStructure = nil;

        // Buffer holding MTLAccelerationStructureInstanceDescriptor array (shared, persistently mapped)
        MetalBuffer*                      m_InstanceBuf  = nullptr;
        void*                             m_InstanceMapped = nullptr;

        // 8-byte buffer containing the AS GPU resource ID — required by Metal Shader Converter
        MetalBuffer*                      m_HandleBuf    = nullptr;
        void*                             m_HandleMapped = nullptr;

        int32                             m_BindlessIndex = -1;
        bool                              m_Built = false;
    };
}
