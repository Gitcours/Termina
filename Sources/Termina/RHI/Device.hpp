#pragma once

#include <Termina/Core/Window.hpp>

#include "Surface.hpp"
#include "RenderContext.hpp"
#include "Texture.hpp"
#include "TextureView.hpp"
#include "RenderPipeline.hpp"
#include "Buffer.hpp"
#include "BufferView.hpp"
#include "Sampler.hpp"
#include "ComputePipeline.hpp"
#include "BLAS.hpp"
#include "TLAS.hpp"

namespace Termina {
    /// Represents the backend renderer used by the device.
    enum class RendererBackend
    {
        Vulkan,
        Metal,
        D3D12
    };

    /// Represents a renderer device that provides access to rendering resources and execution.
    class RendererDevice
    {
    public:
        static RendererDevice* Create();
        virtual ~RendererDevice() = default;

        // Device execution
        virtual void ExecuteRenderContext(RenderContext* context) = 0;
        virtual void WaitIdle() = 0;

        // Device objects
        virtual RendererBackend GetBackend() const = 0;
        virtual RendererSurface* CreateSurface(Window* window) = 0;
        virtual RenderContext* CreateRenderContext(bool singleTime) = 0;
        virtual RendererTexture* CreateTexture(const TextureDesc& desc) = 0;
        virtual TextureView* CreateTextureView(const TextureViewDesc& desc) = 0;
        virtual RenderPipeline* CreateRenderPipeline(const RenderPipelineDesc& desc) = 0;
        virtual RendererBuffer* CreateBuffer(const BufferDesc& desc) = 0;
        virtual BufferView* CreateBufferView(const BufferViewDesc& desc) = 0;
        virtual Sampler* CreateSampler(const SamplerDesc& desc) = 0;
        virtual ComputePipeline* CreateComputePipeline(const ShaderModule& module, const std::string& name = "Compute Pipeline") = 0;
        virtual BLAS* CreateBLAS(const BLASDesc& desc) = 0;
        virtual TLAS* CreateTLAS(uint32 maxInstances) = 0;

        // Device information
        virtual bool SupportsRaytracing() const = 0;
        virtual bool SupportsMeshShaders() const = 0;
        virtual uint64 GetOptimalRowPitchAlignment() const = 0;
        virtual uint64 GetBufferImageGranularity() const = 0;
        virtual TextureFormat GetSurfaceFormat() const = 0;
    };
}
