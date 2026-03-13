#include "GBufferPass.hpp"

#include <Termina/Core/Application.hpp>
#include <Termina/Asset/AssetSystem.hpp>
#include <Termina/Asset/Model/ModelAsset.hpp>
#include <Termina/Asset/Material/MaterialAsset.hpp>
#include <Termina/Renderer/Components/MeshComponent.hpp>
#include <Termina/Renderer/Renderer.hpp>
#include <Termina/Shader/ShaderManager.hpp>
#include <Termina/Shader/ShaderServer.hpp>
#include "World/Components/Transform.hpp"
#include "RHI/TextureView.hpp"

#include <GLM/glm.hpp>

namespace Termina {

    GBufferPass::GBufferPass()
    {
        RendererDevice* device = Application::GetSystem<RendererSystem>()->GetDevice();
        uint32 width  = Application::Get().GetWindow()->GetPixelWidth();
        uint32 height = Application::Get().GetWindow()->GetPixelHeight();

        auto colorRT = TextureUsage::RENDER_TARGET | TextureUsage::SHADER_READ;
        auto floatRT = TextureUsage::RENDER_TARGET | TextureUsage::SHADER_READ;

        m_AlbedoTexture = device->CreateTexture(TextureDesc()
            .SetSize(width, height)
            .SetFormat(TextureFormat::RGBA8_UNORM)
            .SetUsage(colorRT));
        m_AlbedoTexture->SetName("GBuffer Albedo");

        m_NormalsTexture = device->CreateTexture(TextureDesc()
            .SetSize(width, height)
            .SetFormat(TextureFormat::RGBA16_FLOAT)
            .SetUsage(floatRT));
        m_NormalsTexture->SetName("GBuffer Normals");

        m_ORMTexture = device->CreateTexture(TextureDesc()
            .SetSize(width, height)
            .SetFormat(TextureFormat::RGBA8_UNORM)
            .SetUsage(colorRT));
        m_ORMTexture->SetName("GBuffer ORM");

        m_EmissiveTexture = device->CreateTexture(TextureDesc()
            .SetSize(width, height)
            .SetFormat(TextureFormat::RGBA16_FLOAT)
            .SetUsage(floatRT));
        m_EmissiveTexture->SetName("GBuffer Emissive");

        m_MotionVecTexture = device->CreateTexture(TextureDesc()
            .SetSize(width, height)
            .SetFormat(TextureFormat::RGBA16_FLOAT)
            .SetUsage(floatRT));
        m_MotionVecTexture->SetName("GBuffer MotionVectors");

        m_DepthTexture = device->CreateTexture(TextureDesc()
            .SetSize(width, height)
            .SetFormat(TextureFormat::D32_FLOAT)
            .SetUsage(TextureUsage::DEPTH_TARGET | TextureUsage::SHADER_READ));
        m_DepthTexture->SetName("GBuffer Depth");

        m_Sampler = device->CreateSampler(SamplerDesc()
            .SetFilter(SamplerFilter::LINEAR)
            .SetAddress(SamplerAddressMode::WRAP)
            .EnableMips(true));

        RenderPipelineDesc rpDesc = RenderPipelineDesc()
            .SetName("GBuffer Pipeline")
            .AddColorAttachmentFormat(TextureFormat::RGBA8_UNORM)   // albedo
            .AddColorAttachmentFormat(TextureFormat::RGBA16_FLOAT)  // normals
            .AddColorAttachmentFormat(TextureFormat::RGBA8_UNORM)   // ORM
            .AddColorAttachmentFormat(TextureFormat::RGBA16_FLOAT)  // emissive
            .AddColorAttachmentFormat(TextureFormat::RGBA16_FLOAT)  // motion vectors
            .SetDepthAttachmentFormat(TextureFormat::D32_FLOAT)
#if defined(TRMN_MACOS)
            .SetCullMode(PipelineCullMode::FRONT)
#else
            .SetCullMode(PipelineCullMode::BACK)
#endif
            .SetDepthCompareOp(PipelineCompareOp::LESS)
            .SetDepthReadEnabled(true)
            .SetDepthWriteEnabled(true)
            .SetPushConstantSize(128);

        ShaderServer& server = Application::GetSystem<ShaderManager>()->GetShaderServer();
        server.WatchPipeline("Assets/Shaders/GBuffer.hlsl", rpDesc, PipelineType::Graphics);
    }

    GBufferPass::~GBufferPass()
    {
        delete m_Sampler;
        delete m_AlbedoTexture;
        delete m_NormalsTexture;
        delete m_ORMTexture;
        delete m_EmissiveTexture;
        delete m_MotionVecTexture;
        delete m_DepthTexture;
    }

    void GBufferPass::Resize(int32 width, int32 height)
    {
        m_AlbedoTexture->Resize(width, height);
        m_NormalsTexture->Resize(width, height);
        m_ORMTexture->Resize(width, height);
        m_EmissiveTexture->Resize(width, height);
        m_MotionVecTexture->Resize(width, height);
        m_DepthTexture->Resize(width, height);
    }

    void GBufferPass::Execute(RenderPassExecuteInfo& info)
    {
        ShaderServer& server = Application::GetSystem<ShaderManager>()->GetShaderServer();

        auto getRTV = [&](RendererTexture* tex) {
            return info.ViewCache->GetTextureView(
                TextureViewDesc().SetTexture(tex).SetType(TextureViewType::RENDER_TARGET));
        };

        TextureView* albedoRTV   = getRTV(m_AlbedoTexture);
        TextureView* normalsRTV  = getRTV(m_NormalsTexture);
        TextureView* ormRTV      = getRTV(m_ORMTexture);
        TextureView* emissiveRTV = getRTV(m_EmissiveTexture);
        TextureView* motionRTV   = getRTV(m_MotionVecTexture);
        TextureView* depthDTV    = info.ViewCache->GetTextureView(
            TextureViewDesc().SetTexture(m_DepthTexture).SetType(TextureViewType::DEPTH_TARGET));

        RenderEncoderInfo rei = RenderEncoderInfo()
            .SetName("GBuffer Pass")
            .SetDimensions(info.Width, info.Height)
            .AddColorAttachment(albedoRTV,   true, glm::vec4(0.0f))
            .AddColorAttachment(normalsRTV,  true, glm::vec4(0.0f))
            .AddColorAttachment(ormRTV,      true, glm::vec4(0.0f))
            .AddColorAttachment(emissiveRTV, true, glm::vec4(0.0f))
            .AddColorAttachment(motionRTV,   true, glm::vec4(0.0f))
            .SetDepthAttachment(depthDTV);

        RenderEncoder* re = info.Ctx->CreateRenderEncoder(rei);
        re->SetPipeline(server.GetPipeline("Assets/Shaders/GBuffer.hlsl"));
        re->SetViewport(0.0f, 0.0f, static_cast<float>(info.Width), static_cast<float>(info.Height));
        re->SetScissorRect(0, 0, info.Width, info.Height);

        struct GBufferPushConstants
        {
            glm::mat4 MVP;
            int32 VertexBufferIndex;
            int32 BaseVertex;
            int32 AlbedoIndex;
            int32 NormalIndex;
            int32 ORMIndex;
            int32 EmissiveIndex;
            int32 SamplerIndex;
            int32 _pad;
        };

        const int32 samplerIndex = m_Sampler->GetBindlessHandle();

        auto resolveTextureIndex = [&](const AssetHandle<TextureAsset>& handle) -> int32 {
            if (!handle.IsValid()) return -1;
            RendererTexture* tex = handle->GetTexture();
            if (!tex) return -1;
            TextureView* tv = info.ViewCache->GetTextureView(
                TextureViewDesc::CreateDefault(tex, TextureViewType::SHADER_READ, TextureViewDimension::TEXTURE_2D));
            return tv->GetBindlessIndex();
        };

        for (auto& actor : info.CurrentWorld->GetActors())
        {
            if (!actor->HasComponent<MeshComponent>()) continue;

            auto& meshComp = actor->GetComponent<MeshComponent>();
            ModelAsset* model = meshComp.Model.Get();
            if (!model || !model->VertexBuffer || !model->IndexBuffer || !model->VertexView)
                continue;

            re->SetIndexBuffer(model->IndexBuffer);

            const int32 vbIndex = model->VertexView->GetBindlessHandle();
            const glm::mat4 actorWorld = actor->GetComponent<Transform>().GetWorldMatrix();

            for (const MeshInstance& inst : model->Instances)
            {
                if (inst.LODs.empty()) continue;
                const MeshLOD& lod = inst.LODs[0];

                int32 albedoIndex   = -1;
                int32 normalIndex   = -1;
                int32 ormIndex      = -1;
                int32 emissiveIndex = -1;

                if (inst.MaterialIndex < model->Materials.size())
                {
                    MaterialAsset* mat = model->Materials[inst.MaterialIndex].Get();
                    if (mat)
                    {
                        const Material& m = mat->GetMaterial();
                        albedoIndex   = resolveTextureIndex(m.AlbedoTexture);
                        normalIndex   = resolveTextureIndex(m.NormalTexture);
                        ormIndex      = resolveTextureIndex(m.ORMTexture);
                        emissiveIndex = resolveTextureIndex(m.EmissiveTexture);
                    }
                }

                GBufferPushConstants pc;
                pc.MVP               = info.CurrentCamera.ViewProjection * actorWorld * inst.LocalTransform;
                pc.VertexBufferIndex = vbIndex;
                pc.BaseVertex        = static_cast<int32>(inst.BaseVertex);
                pc.AlbedoIndex       = albedoIndex;
                pc.NormalIndex       = normalIndex;
                pc.ORMIndex          = ormIndex;
                pc.EmissiveIndex     = emissiveIndex;
                pc.SamplerIndex      = samplerIndex;
                pc._pad              = 0;

                re->SetConstants(sizeof(pc), &pc);
                re->DrawIndexed(lod.IndexCount, 1, lod.IndexOffset, 0, 0);
            }
        }

        re->End();

        // Register GBuffer outputs
        info.IO->RegisterTexture("GBuffer_Albedo",        m_AlbedoTexture);
        info.IO->RegisterTexture("GBuffer_Normals",       m_NormalsTexture);
        info.IO->RegisterTexture("GBuffer_ORM",           m_ORMTexture);
        info.IO->RegisterTexture("GBuffer_Emissive",      m_EmissiveTexture);
        info.IO->RegisterTexture("GBuffer_MotionVectors", m_MotionVecTexture);
        info.IO->RegisterTexture("GBuffer_Depth",         m_DepthTexture);

        // Transition all color targets to shader-readable
        auto colorBarrier = [&](RendererTexture* tex) {
            info.Ctx->Barrier(TextureBarrier()
                .SetTargetTexture(tex)
                .SetNewLayout(TextureLayout::READ_ONLY)
                .SetDstAccess(ResourceAccess::SHADER_READ)
                .SetDstStage(PipelineStage::PIXEL_SHADER));
        };
        colorBarrier(m_AlbedoTexture);
        colorBarrier(m_NormalsTexture);
        colorBarrier(m_ORMTexture);
        colorBarrier(m_EmissiveTexture);
        colorBarrier(m_MotionVecTexture);
    }

} // namespace Termina
