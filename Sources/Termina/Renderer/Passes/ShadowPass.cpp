#include "ShadowPass.hpp"

#include <Termina/Core/Application.hpp>
#include <Termina/Asset/Model/ModelAsset.hpp>
#include <Termina/Renderer/Renderer.hpp>
#include <Termina/Renderer/Components/MeshComponent.hpp>
#include <Termina/Renderer/GPULight.hpp>
#include <Termina/Shader/ShaderManager.hpp>
#include <Termina/Shader/ShaderServer.hpp>
#include <Termina/RHI/TextureView.hpp>
#include <World/Components/Transform.hpp>

#include <GLM/glm.hpp>

namespace Termina {

    ShadowPass::ShadowPass()
    {
        RendererDevice* device = Application::GetSystem<RendererSystem>()->GetDevice();

        uint32 width  = Application::Get().GetWindow()->GetPixelWidth();
        uint32 height = Application::Get().GetWindow()->GetPixelHeight();

        m_ShadowMask = device->CreateTexture(TextureDesc()
            .SetSize(width, height)
            .SetFormat(TextureFormat::R8_UNORM)
            .SetUsage(TextureUsage::SHADER_WRITE | TextureUsage::SHADER_READ | TextureUsage::RENDER_TARGET));
        m_ShadowMask->SetName("Shadow Mask");

        if (device->SupportsRaytracing())
        {
            m_TLAS = device->CreateTLAS(MAX_TLAS_INSTANCES);

            // Pre-allocate TLAS scratch buffer (~4 MB is sufficient for most scenes)
            m_TLASScratch = device->CreateBuffer(BufferDesc()
                .SetSize(4 * 1024 * 1024)
                .SetUsage(BufferUsage::ACCELERATION_STRUCTURE | BufferUsage::SHADER_WRITE));
            m_TLASScratch->SetName("TLAS Scratch");

            ShaderServer& server = Application::GetSystem<ShaderManager>()->GetShaderServer();
            server.WatchPipeline("__TERMINA__/CORE_SHADERS/RTShadow.hlsl", PipelineType::Compute);
        }
    }

    ShadowPass::~ShadowPass()
    {
        delete m_TLASScratch;
        delete m_TLAS;
        delete m_ShadowMask;
    }

    void ShadowPass::Resize(int32 width, int32 height)
    {
        if (m_ShadowMask)
            m_ShadowMask->Resize(width, height);
    }

    void ShadowPass::Execute(RenderPassExecuteInfo& info)
    {
        if (info.Device->SupportsRaytracing() && m_TLAS)
        {
            ExecuteRTShadow(info);
        }
        else
        {
            ExecuteCSMShadow(info);
        }

        info.IO->RegisterTexture("ShadowMask", m_ShadowMask);
    }

    void ShadowPass::ExecuteRTShadow(RenderPassExecuteInfo& info)
    {
        ShaderServer& server = Application::GetSystem<ShaderManager>()->GetShaderServer();
        ComputePipeline* pipeline = server.GetComputePipeline("__TERMINA__/CORE_SHADERS/RTShadow.hlsl");
        if (!pipeline) return;

        // -----------------------------------------------------------------------
        // Collect TLAS instances from all actors with a valid BLAS
        // -----------------------------------------------------------------------
        std::vector<TLASInstanceDesc> tlasInstances;
        tlasInstances.reserve(MAX_TLAS_INSTANCES);

        uint32 instanceID = 0;
        for (auto& actor : info.CurrentWorld->GetActors())
        {
            if (!actor->HasComponent<MeshComponent>()) continue;

            auto& meshComp = actor->GetComponent<MeshComponent>();
            ModelAsset* model = meshComp.Model.Get();
            if (!model || !model->BLASObject) continue;

            const glm::mat4 worldMatrix = actor->GetComponent<Transform>().GetWorldMatrix();

            TLASInstanceDesc inst;
            inst.BLASObject = model->BLASObject;
            inst.Transform  = worldMatrix;
            inst.InstanceID = instanceID++;
            inst.Mask       = 0xFF;
            inst.Opaque     = true;

            tlasInstances.push_back(inst);

            if (tlasInstances.size() >= MAX_TLAS_INSTANCES) break;
        }

        if (tlasInstances.empty())
            return;

        // -----------------------------------------------------------------------
        // Build TLAS (barrier: AS build write → compute read)
        // -----------------------------------------------------------------------
        m_TLAS->Build(info.Ctx, tlasInstances, m_TLASScratch, 0);

        info.Ctx->Barrier(BufferBarrier()
            .SetTargetBuffer(m_TLASScratch)
            .SetDstStage(PipelineStage::COMPUTE_SHADER)
            .SetDstAccess(ResourceAccess::ACCELERATION_STRUCTURE_READ));

        // -----------------------------------------------------------------------
        // Set shadow mask to GENERAL for UAV write
        // -----------------------------------------------------------------------
        info.Ctx->Barrier(TextureBarrier()
            .SetTargetTexture(m_ShadowMask)
            .SetNewLayout(TextureLayout::GENERAL)
            .SetDstAccess(ResourceAccess::SHADER_WRITE)
            .SetDstStage(PipelineStage::COMPUTE_SHADER));

        // -----------------------------------------------------------------------
        // Dispatch RTShadow compute shader
        // -----------------------------------------------------------------------
        TextureView* shadowUAV = info.ViewCache->GetTextureView(
            TextureViewDesc::CreateDefault(m_ShadowMask, TextureViewType::SHADER_WRITE, TextureViewDimension::TEXTURE_2D));

        RendererTexture* depthTex   = info.IO->GetTexture("GBuffer_Depth");
        RendererTexture* normalsTex = info.IO->GetTexture("GBuffer_Normals");

        TextureView* depthSRV = info.ViewCache->GetTextureView(
            TextureViewDesc::CreateDefault(depthTex, TextureViewType::SHADER_READ, TextureViewDimension::TEXTURE_2D));
        TextureView* normalsSRV = info.ViewCache->GetTextureView(
            TextureViewDesc::CreateDefault(normalsTex, TextureViewType::SHADER_READ, TextureViewDimension::TEXTURE_2D));

        // Count directional lights — shader traces rays only when at least one is present
        int32 lightCount = 0;
        if (info.LightList)
        {
            for (const GPULight& l : *info.LightList)
                if (l.Type == (int32)LightType::Directional) { ++lightCount; }
        }

        struct RTShadowPushConstants
        {
            int32       DepthIndex;
            int32       NormalsIndex;
            int32       OutputIndex;
            int32       TLASIndex;
            int32       LightBufferIndex;
            int32       LightCount;
            int32       Width;
            int32       Height;
            glm::mat4   InvViewProj; // column_major
            glm::vec3   CameraPos;
            int32       _pad;
            glm::vec3   LightDir;    // normalized direction *toward* primary directional light
            int32       _pad2;
        };
        static_assert(sizeof(RTShadowPushConstants) == 128, "RTShadowPushConstants must be 128 bytes");

        // Find first directional light and extract its direction toward the light (-Direction)
        glm::vec3 lightDir(0.0f, -1.0f, 0.0f); // default: straight down
        if (info.LightList)
        {
            for (const GPULight& l : *info.LightList)
            {
                if (l.Type == (int32)LightType::Directional)
                {
                    lightDir = -glm::normalize(l.Direction);
                    break;
                }
            }
        }

        RTShadowPushConstants pc;
        pc.DepthIndex       = depthSRV->GetBindlessIndex();
        pc.NormalsIndex     = normalsSRV->GetBindlessIndex();
        pc.OutputIndex      = shadowUAV->GetBindlessIndex();
        pc.TLASIndex        = m_TLAS->GetBindlessIndex();
        pc.LightBufferIndex = -1;
        pc.LightCount       = lightCount;
        pc.Width            = info.Width;
        pc.Height           = info.Height;
        pc.InvViewProj      = info.CurrentCamera.InverseViewProjection;
        pc.CameraPos        = info.CurrentCamera.Position;
        pc._pad             = 0;
        pc.LightDir         = lightDir;
        pc._pad2            = 0;

        ComputeEncoder* ce = info.Ctx->CreateComputeEncoder("RT Shadow Pass");
        ce->SetPipeline(pipeline);
        ce->SetConstants(sizeof(pc), &pc);
        ce->Dispatch((info.Width + 7) / 8, (info.Height + 7) / 8, 1, 8, 8, 1);
        ce->End();

        // -----------------------------------------------------------------------
        // Transition shadow mask to READ_ONLY for DeferredPass
        // -----------------------------------------------------------------------
        info.Ctx->Barrier(TextureBarrier()
            .SetTargetTexture(m_ShadowMask)
            .SetNewLayout(TextureLayout::READ_ONLY)
            .SetDstAccess(ResourceAccess::SHADER_READ)
            .SetDstStage(PipelineStage::COMPUTE_SHADER));
    }

    void ShadowPass::ExecuteCSMShadow(RenderPassExecuteInfo& info)
    {
        // No-op CSM implementation for now: clear the shadow mask to 1.0 (white/lit)
        TextureView* shadowRTV = info.ViewCache->GetTextureView(
            TextureViewDesc::CreateDefault(m_ShadowMask, TextureViewType::RENDER_TARGET, TextureViewDimension::TEXTURE_2D));

        RenderEncoderInfo rei = RenderEncoderInfo()
            .SetName("CSM Shadow Clear")
            .SetDimensions(info.Width, info.Height)
            .AddColorAttachment(shadowRTV, true, glm::vec4(1.0f));

        RenderEncoder* re = info.Ctx->CreateRenderEncoder(rei);
        re->End();

        info.Ctx->Barrier(TextureBarrier()
            .SetTargetTexture(m_ShadowMask)
            .SetNewLayout(TextureLayout::READ_ONLY)
            .SetDstAccess(ResourceAccess::SHADER_READ)
            .SetDstStage(PipelineStage::ALL_GRAPHICS));
    }

} // namespace Termina
