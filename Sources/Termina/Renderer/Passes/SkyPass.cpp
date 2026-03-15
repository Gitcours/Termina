#include "SkyPass.hpp"

#include <Termina/Core/Application.hpp>
#include <Termina/Renderer/Renderer.hpp>
#include <Termina/Shader/ShaderManager.hpp>
#include <Termina/Shader/ShaderServer.hpp>
#include "RHI/TextureView.hpp"
#include <Termina/Renderer/GPULight.hpp>

#include <GLM/glm.hpp>

namespace Termina {

    SkyPass::SkyPass()
    {
        ShaderServer& server = Application::GetSystem<ShaderManager>()->GetShaderServer();
        server.WatchPipeline("__TERMINA__/CORE_SHADERS/Sky.hlsl", PipelineType::Compute);
    }

    SkyPass::~SkyPass()
    {
    }

    void SkyPass::Resize(int32 width, int32 height)
    {
        // SkyPass does not own any textures, so nothing to resize.
    }

    void SkyPass::Execute(RenderPassExecuteInfo& info)
    {
        ShaderServer& server = Application::GetSystem<ShaderManager>()->GetShaderServer();

        RendererTexture* hdrTex = info.IO->GetTexture("HDRColor");
        RendererTexture* depthTex = info.IO->GetTexture("GBuffer_Depth");

        TextureView* hdrUAV = info.ViewCache->GetTextureView(
            TextureViewDesc::CreateDefault(hdrTex, TextureViewType::SHADER_WRITE, TextureViewDimension::TEXTURE_2D));
        TextureView* depthSRV = info.ViewCache->GetTextureView(
            TextureViewDesc::CreateDefault(depthTex, TextureViewType::SHADER_READ, TextureViewDimension::TEXTURE_2D));

        struct SkyPushConstants
        {
            int32 OutputIndex;
            int32 DepthIndex;
            int32 Width;
            int32 Height;
            glm::mat4 InvViewProj;
            glm::vec3 CameraPos;
            int32 _pad;
            glm::vec3 SunDir;
            int32 _pad2;
        };

        glm::vec3 sunDir = glm::normalize(glm::vec3(0.2f, 0.8f, -0.5f));
        if (info.LightList)
        {
            for (const GPULight& l : *info.LightList)
            {
                if (l.Type == (int32)LightType::Directional)
                {
                    sunDir = -glm::normalize(l.Direction);
                    break;
                }
            }
        }

        SkyPushConstants pc;
        pc.OutputIndex = hdrUAV->GetBindlessIndex();
        pc.DepthIndex  = depthSRV->GetBindlessIndex();
        pc.Width       = info.Width;
        pc.Height      = info.Height;
        pc.InvViewProj = info.CurrentCamera.InverseViewProjection;
        pc.CameraPos   = info.CurrentCamera.Position;
        pc._pad        = 0;
        pc.SunDir      = sunDir;
        pc._pad2       = 0;

        // Transition HDR texture to general/write access for the compute shader
        info.Ctx->Barrier(TextureBarrier()
            .SetTargetTexture(hdrTex)
            .SetNewLayout(TextureLayout::GENERAL)
            .SetDstAccess(ResourceAccess::SHADER_WRITE)
            .SetDstStage(PipelineStage::COMPUTE_SHADER));

        ComputeEncoder* ce = info.Ctx->CreateComputeEncoder("Sky Pass");
        ce->SetPipeline(server.GetComputePipeline("__TERMINA__/CORE_SHADERS/Sky.hlsl"));
        ce->SetConstants(sizeof(pc), &pc);
        ce->Dispatch(info.Width, info.Height, 1, 8, 8, 1);
        ce->End();

        // Restore layout for the subsequent passes (e.g., Tonemap)
        info.Ctx->Barrier(TextureBarrier()
            .SetTargetTexture(hdrTex)
            .SetNewLayout(TextureLayout::READ_ONLY)
            .SetDstAccess(ResourceAccess::SHADER_READ)
            .SetDstStage(PipelineStage::COMPUTE_SHADER));
    }

} // namespace Termina