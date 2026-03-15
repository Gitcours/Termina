#pragma once

#include <Termina/Renderer/RenderPass.hpp>
#include <Termina/RHI/Texture.hpp>
#include <Termina/RHI/Buffer.hpp>
#include <Termina/RHI/TLAS.hpp>

namespace Termina {

    /// Raytraced hard shadow pass.
    /// Runs after GBuffer, outputs R8_UNORM shadow mask (1=lit, 0=shadowed).
    /// Sampled by DeferredPass for directional light evaluation.
    /// No-ops gracefully on devices that don't support raytracing.
    class RTShadowPass : public RenderPass
    {
    public:
        RTShadowPass();
        ~RTShadowPass() override;

        void Resize(int32 width, int32 height) override;
        void Execute(RenderPassExecuteInfo& info) override;

    private:
        static constexpr uint32 MAX_TLAS_INSTANCES = 4096;

        RendererTexture* m_ShadowMask  = nullptr; // R8_UNORM, SHADER_WRITE | SHADER_READ
        TLAS*            m_TLAS        = nullptr;
        RendererBuffer*  m_TLASScratch = nullptr; // pre-allocated scratch for TLAS rebuild
    };

} // namespace Termina
