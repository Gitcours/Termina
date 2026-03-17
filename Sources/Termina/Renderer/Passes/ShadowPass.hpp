#pragma once

#include <Termina/Renderer/RenderPass.hpp>
#include <Termina/RHI/Texture.hpp>
#include <Termina/RHI/Buffer.hpp>
#include <Termina/RHI/TLAS.hpp>

namespace Termina {

    /// Directional shadow pass.
    /// Supports Raytraced hard shadows or Cascaded Shadow Mapping (CSM).
    /// Runs after GBuffer, outputs R8Reg_UNORM shadow mask (1=lit, 0=shadowed).
    /// Sampled by DeferredPass for directional light evaluation.
    class ShadowPass : public RenderPass
    {
    public:
        ShadowPass();
        ~ShadowPass() override;

        void Resize(int32 width, int32 height) override;
        void Execute(RenderPassExecuteInfo& info) override;

    private:
        void ExecuteRTShadow(RenderPassExecuteInfo& info);
        void ExecuteCSMShadow(RenderPassExecuteInfo& info);

        static constexpr uint32 MAX_TLAS_INSTANCES = 4096;

        RendererTexture* m_ShadowMask  = nullptr; // R8_UNORM, SHADER_WRITE | SHADER_READ
        TLAS*            m_TLAS        = nullptr;
        RendererBuffer*  m_TLASScratch = nullptr; // pre-allocated scratch for TLAS rebuild
    };

} // namespace Termina
