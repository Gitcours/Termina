#include "DeferredPass.hpp"

#include <Termina/Core/Application.hpp>
#include <Termina/Renderer/Renderer.hpp>
#include <Termina/Renderer/GPULight.hpp>
#include <Termina/Shader/ShaderManager.hpp>
#include <Termina/Shader/ShaderServer.hpp>
#include "RHI/TextureView.hpp"

#include <GLM/glm.hpp>
#include <algorithm>
#include <cstring>

namespace Termina {

    DeferredPass::DeferredPass()
    {
        RendererDevice* device = Application::GetSystem<RendererSystem>()->GetDevice();
        uint32 width  = Application::Get().GetWindow()->GetPixelWidth();
        uint32 height = Application::Get().GetWindow()->GetPixelHeight();

        m_HDRTexture = device->CreateTexture(TextureDesc()
            .SetSize(width, height)
            .SetFormat(TextureFormat::RGBA16_FLOAT)
            .SetUsage(TextureUsage::SHADER_READ | TextureUsage::SHADER_WRITE | TextureUsage::RENDER_TARGET));
        m_HDRTexture->SetName("HDR Color");

        m_LightBuffer = device->CreateBuffer(BufferDesc()
            .SetSize(sizeof(GPULight) * MAX_LIGHTS)
            .SetStride(sizeof(GPULight))
            .SetUsage(BufferUsage::SHADER_READ | BufferUsage::TRANSFER));
        m_LightBuffer->SetName("Light Buffer");
        m_LightMapped = m_LightBuffer->Map();

        m_LightBufView = device->CreateBufferView(BufferViewDesc()
            .SetBuffer(m_LightBuffer)
            .SetType(BufferViewType::SHADER_READ));

        ShaderServer& server = Application::GetSystem<ShaderManager>()->GetShaderServer();
        server.WatchPipeline("__TERMINA__/CORE_SHADERS/Deferred.hlsl", PipelineType::Compute);
    }

    DeferredPass::~DeferredPass()
    {
        m_LightBuffer->Unmap();
        delete m_LightBufView;
        delete m_LightBuffer;
        delete m_HDRTexture;
    }

    void DeferredPass::Resize(int32 width, int32 height)
    {
        m_HDRTexture->Resize(width, height);
    }

    void DeferredPass::Execute(RenderPassExecuteInfo& info)
    {
        ShaderServer& server = Application::GetSystem<ShaderManager>()->GetShaderServer();

        // Write light data directly into the persistently-mapped buffer
        int32 lightCount = 0;
        if (info.LightList && !info.LightList->empty())
        {
            lightCount = static_cast<int32>(
                std::min(static_cast<size_t>(MAX_LIGHTS), info.LightList->size()));
            std::memcpy(m_LightMapped, info.LightList->data(),
                        static_cast<size_t>(lightCount) * sizeof(GPULight));
        }

        auto getSRV = [&](RendererTexture* tex) {
            return info.ViewCache->GetTextureView(
                TextureViewDesc::CreateDefault(tex, TextureViewType::SHADER_READ, TextureViewDimension::TEXTURE_2D));
        };

        TextureView* hdrUAV = info.ViewCache->GetTextureView(
            TextureViewDesc::CreateDefault(m_HDRTexture, TextureViewType::SHADER_WRITE, TextureViewDimension::TEXTURE_2D));

        RendererTexture* depthTex = info.IO->GetTexture("GBuffer_Depth");
        TextureView* depthSRV = info.ViewCache->GetTextureView(
            TextureViewDesc::CreateDefault(depthTex, TextureViewType::SHADER_READ, TextureViewDimension::TEXTURE_2D));

        struct DeferredPushConstants
        {
            int32       AlbedoIndex;
            int32       NormalsIndex;
            int32       ORMIndex;
            int32       EmissiveIndex;
            int32       DepthIndex;
            int32       OutputIndex;
            int32       LightBufferIndex;
            int32       LightCount;
            glm::mat4   InvViewProj;   // column_major
            glm::vec3   CameraPos;
            int32       Width;
            int32       Height;
            int32       ShadowMaskIndex;
        };

        DeferredPushConstants pc;
        pc.AlbedoIndex      = getSRV(info.IO->GetTexture("GBuffer_Albedo"))->GetBindlessIndex();
        pc.NormalsIndex     = getSRV(info.IO->GetTexture("GBuffer_Normals"))->GetBindlessIndex();
        pc.ORMIndex         = getSRV(info.IO->GetTexture("GBuffer_ORM"))->GetBindlessIndex();
        pc.EmissiveIndex    = getSRV(info.IO->GetTexture("GBuffer_Emissive"))->GetBindlessIndex();
        pc.DepthIndex       = depthSRV->GetBindlessIndex();
        pc.OutputIndex      = hdrUAV->GetBindlessIndex();
        pc.LightBufferIndex = m_LightBufView->GetBindlessHandle();
        pc.LightCount       = lightCount;
        pc.InvViewProj      = info.CurrentCamera.InverseViewProjection;
        pc.CameraPos        = info.CurrentCamera.Position;
        pc.Width            = info.Width;
        pc.Height           = info.Height;
        pc.ShadowMaskIndex  = -1;
        if (info.IO->HasTexture("ShadowMask"))
        {
            RendererTexture* shadowMask = info.IO->GetTexture("ShadowMask");
            TextureView* shadowSRV = info.ViewCache->GetTextureView(
                TextureViewDesc::CreateDefault(shadowMask, TextureViewType::SHADER_READ, TextureViewDimension::TEXTURE_2D));
            pc.ShadowMaskIndex = shadowSRV->GetBindlessIndex();
        }

        ComputeEncoder* ce = info.Ctx->CreateComputeEncoder("Deferred Pass");
        ce->SetPipeline(server.GetComputePipeline("__TERMINA__/CORE_SHADERS/Deferred.hlsl"));
        ce->SetConstants(sizeof(pc), &pc);
        ce->Dispatch(info.Width, info.Height, 1, 8, 8, 1);
        ce->End();

        info.IO->RegisterTexture("HDRColor", m_HDRTexture);

        info.Ctx->Barrier(TextureBarrier()
            .SetTargetTexture(m_HDRTexture)
            .SetNewLayout(TextureLayout::READ_ONLY)
            .SetDstAccess(ResourceAccess::SHADER_READ)
            .SetDstStage(PipelineStage::COMPUTE_SHADER));
    }

} // namespace Termina
