#pragma once

#include <Termina/Renderer/RenderPass.hpp>
#include <Termina/RHI/Buffer.hpp>
#include <Termina/RHI/BufferView.hpp>
#include <Termina/RHI/Sampler.hpp>
#include <Termina/Renderer/Camera.hpp>
#include <GLM/glm.hpp>

namespace Termina {

    /// Geometry pass: renders all mesh actors into the GBuffer MRTs.
    /// Builds the global instance and material buffers each frame from the current world.
    class GBufferPass : public RenderPass
    {
    public:
        GBufferPass();
        ~GBufferPass() override;

        void Resize(int32 width, int32 height) override;
        void Execute(RenderPassExecuteInfo& info) override;
        void Inspect() override;

        int32 GetLastDrawCallCount() const { return m_LastDrawCallCount; }
        int32 GetLastTriangleCount() const { return m_LastTriangleCount; }

    private:
        static constexpr int32 MAX_INSTANCES = 4096;
        static constexpr int32 MAX_MATERIALS = 1024;

        // GBuffer render targets
        RendererTexture* m_AlbedoTexture       = nullptr; // RGBA8_UNORM
        RendererTexture* m_NormalsTexture      = nullptr; // RGBA16_FLOAT
        RendererTexture* m_ORMTexture          = nullptr; // RGBA8_UNORM
        RendererTexture* m_EmissiveTexture     = nullptr; // RGBA16_FLOAT
        RendererTexture* m_MotionVecTexture    = nullptr; // RGBA16_FLOAT
        RendererTexture* m_DepthTexture        = nullptr; // D32_FLOAT
        Sampler*         m_Sampler             = nullptr;

        // Global scene buffers (eCpuToGpu, persistently mapped)
        RendererBuffer*  m_InstanceBuffer      = nullptr;
        BufferView*      m_InstanceBufView     = nullptr;
        void*            m_InstanceMapped      = nullptr;

        RendererBuffer*  m_MaterialBuffer      = nullptr;
        BufferView*      m_MaterialBufView     = nullptr;
        void*            m_MaterialMapped      = nullptr;

        // Frustum culling
        bool             m_FreezeFrustum       = false;
        bool             m_FrustumFrozen       = false;  // true once we have a stored frustum
        glm::mat4        m_FrozenViewProj      = glm::mat4(1.0f);
        int32            m_LastCulledCount     = 0;
        int32            m_LastTotalCount      = 0;

        // Rendering stats
        int32            m_LastDrawCallCount   = 0;
        int32            m_LastTriangleCount   = 0;
    };

} // namespace Termina
