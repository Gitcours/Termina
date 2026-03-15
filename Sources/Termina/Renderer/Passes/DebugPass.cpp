#include "DebugPass.hpp"
#include "RHI/RenderPipeline.hpp"
#include "RHI/TextureView.hpp"

#include <Termina/Core/Application.hpp>
#include <Termina/Renderer/Renderer.hpp>
#include <Termina/Shader/ShaderManager.hpp>
#include <Termina/Shader/ShaderServer.hpp>

#include <GLM/glm.hpp>
#include <GLM/gtc/constants.hpp>
#include <GLM/gtc/matrix_transform.hpp>

namespace Termina {

    DebugPass* DebugPass::s_Instance = nullptr;
    std::vector<DebugPass::DebugVertex> DebugPass::s_Vertices;

    DebugPass::DebugPass()
    {
        s_Instance = this;
        s_Vertices.reserve(100000);

        RendererDevice* device = Application::GetSystem<RendererSystem>()->GetDevice();

        // Create sampler
        m_Sampler = device->CreateSampler(SamplerDesc()
            .SetFilter(SamplerFilter::LINEAR)
            .SetAddress(SamplerAddressMode::CLAMP));

        // Create pipeline
        ShaderServer& server = Application::GetSystem<ShaderManager>()->GetShaderServer();

        RenderPipelineDesc pipelineDesc = RenderPipelineDesc()
            .SetName("Debug Pipeline")
            .SetCullMode(PipelineCullMode::NONE)
            .SetTopology(PipelineTopology::LINES)
            .SetEnableBlending(true)
            .SetDepthReadEnabled(true)
            .SetDepthWriteEnabled(false)
            .SetDepthCompareOp(PipelineCompareOp::LESS)
            .SetDepthAttachmentFormat(TextureFormat::D32_FLOAT)
            .AddColorAttachmentFormat(TextureFormat::RGBA8_UNORM);

        server.WatchPipeline("__TERMINA__/CORE_SHADERS/Debug.hlsl", pipelineDesc, PipelineType::Graphics);

        // Initialize frame resources
        m_FrameResources.resize(FRAMES_IN_FLIGHT);
    }

    DebugPass::~DebugPass()
    {
        for (auto& resource : m_FrameResources) {
            if (resource.VertexBufferView) delete resource.VertexBufferView;
            if (resource.VertexBuffer) delete resource.VertexBuffer;
        }
        if (m_Sampler) delete m_Sampler;
        s_Instance = nullptr;
    }

    void DebugPass::Resize(int32 width, int32 height)
    {
        // Debug pass doesn't need resizing
    }

    void DebugPass::Execute(RenderPassExecuteInfo& info)
    {
        if (s_Vertices.empty()) {
            return;
        }

        uint32 frameIndex = info.FrameIndex;
        FrameResource& resource = m_FrameResources[frameIndex];

        uint32 requiredSize = static_cast<uint32>(s_Vertices.size());

        // Allocate or resize vertex buffer if needed
        if (resource.VertexBuffer == nullptr || resource.VertexBufferSize < requiredSize) {
            if (resource.VertexBufferView) delete resource.VertexBufferView;
            if (resource.VertexBuffer) delete resource.VertexBuffer;

            resource.VertexBufferSize = requiredSize + 10000;
            resource.VertexBuffer = info.Device->CreateBuffer(BufferDesc()
                .SetSize(resource.VertexBufferSize * sizeof(DebugVertex))
                .SetUsage(BufferUsage::SHADER_READ | BufferUsage::TRANSFER)
                .SetStride(sizeof(DebugVertex)));
            resource.VertexBuffer->SetName("Debug Vertex Buffer");

            resource.VertexBufferView = info.Device->CreateBufferView(BufferViewDesc()
                .SetBuffer(resource.VertexBuffer)
                .SetType(BufferViewType::SHADER_READ));
        }

        // Upload vertex data
        void* vertexData = resource.VertexBuffer->Map();
        memcpy(vertexData, s_Vertices.data(), s_Vertices.size() * sizeof(DebugVertex));
        resource.VertexBuffer->Unmap();

        // Setup render encoder
        RendererTexture* outputTex = info.IO->GetTexture("RendererOutput");
        TextureView* outputView = info.ViewCache->GetTextureView(
            TextureViewDesc().SetTexture(outputTex).SetType(TextureViewType::RENDER_TARGET));

        RenderEncoderInfo encoderInfo = RenderEncoderInfo()
            .SetName("Debug Pass")
            .SetDimensions(info.Width, info.Height)
            .AddColorAttachment(outputView, false);

        RendererTexture* depthTex = info.IO->GetTexture("GBuffer_Depth");
        if (depthTex)
        {
            TextureView* depthView = info.ViewCache->GetTextureView(
                TextureViewDesc().SetTexture(depthTex).SetType(TextureViewType::DEPTH_TARGET));
            encoderInfo.SetDepthAttachment(depthView, false);
        }

        RenderEncoder* encoder = info.Ctx->CreateRenderEncoder(encoderInfo);

        ShaderServer& server = Application::GetSystem<ShaderManager>()->GetShaderServer();
        RenderPipeline* pipeline = server.GetPipeline("__TERMINA__/CORE_SHADERS/Debug.hlsl");

        encoder->SetPipeline(pipeline);
        encoder->SetViewport(0.0f, 0.0f, static_cast<float>(info.Width), static_cast<float>(info.Height));
        encoder->SetScissorRect(0, 0, info.Width, info.Height);

        struct DebugPushConstants
        {
            glm::mat4 ViewProjection;
            int32 VertexBufferIndex;
        };

        DebugPushConstants pc;
        pc.ViewProjection = info.CurrentCamera.ViewProjection;
        pc.VertexBufferIndex = resource.VertexBufferView->GetBindlessHandle();

        encoder->SetConstants(sizeof(pc), &pc);
        encoder->Draw(static_cast<uint32>(s_Vertices.size()), 1, 0, 0);
        encoder->End();

        // Clear vertices for next frame
        s_Vertices.clear();
    }

    glm::vec3 DebugPass::TransformPosition(const glm::vec3& position, const glm::mat4* transform)
    {
        if (transform == nullptr) {
            return position;
        }
        return glm::vec3(*transform * glm::vec4(position, 1.0f));
    }

    void DebugPass::AddVertex(const glm::vec3& position, const glm::vec4& color)
    {
        if (s_Vertices.size() >= MAX_DEBUG_VERTICES) return;
        s_Vertices.push_back({position, color});
    }

    void DebugPass::AddLine(const glm::vec3& start, const glm::vec3& end, const glm::vec4& color, const glm::mat4* transform)
    {
        if (s_Vertices.size() + 2 > MAX_DEBUG_VERTICES) return;
        AddVertex(TransformPosition(start, transform), color);
        AddVertex(TransformPosition(end, transform), color);
    }

    void DebugPass::DrawLine(const glm::vec3& start, const glm::vec3& end, const glm::vec4& color, const glm::mat4* transform)
    {
        if (!s_Instance) return;
        s_Instance->AddLine(start, end, color, transform);
    }

    void DebugPass::DrawBox(const glm::vec3& center, const glm::vec3& extents, const glm::vec4& color, const glm::mat4* transform)
    {
        if (!s_Instance) return;

        glm::vec3 min = center - extents;
        glm::vec3 max = center + extents;

        // Bottom face
        s_Instance->AddLine(glm::vec3(min.x, min.y, min.z), glm::vec3(max.x, min.y, min.z), color, transform);
        s_Instance->AddLine(glm::vec3(max.x, min.y, min.z), glm::vec3(max.x, min.y, max.z), color, transform);
        s_Instance->AddLine(glm::vec3(max.x, min.y, max.z), glm::vec3(min.x, min.y, max.z), color, transform);
        s_Instance->AddLine(glm::vec3(min.x, min.y, max.z), glm::vec3(min.x, min.y, min.z), color, transform);

        // Top face
        s_Instance->AddLine(glm::vec3(min.x, max.y, min.z), glm::vec3(max.x, max.y, min.z), color, transform);
        s_Instance->AddLine(glm::vec3(max.x, max.y, min.z), glm::vec3(max.x, max.y, max.z), color, transform);
        s_Instance->AddLine(glm::vec3(max.x, max.y, max.z), glm::vec3(min.x, max.y, max.z), color, transform);
        s_Instance->AddLine(glm::vec3(min.x, max.y, max.z), glm::vec3(min.x, max.y, min.z), color, transform);

        // Vertical edges
        s_Instance->AddLine(glm::vec3(min.x, min.y, min.z), glm::vec3(min.x, max.y, min.z), color, transform);
        s_Instance->AddLine(glm::vec3(max.x, min.y, min.z), glm::vec3(max.x, max.y, min.z), color, transform);
        s_Instance->AddLine(glm::vec3(max.x, min.y, max.z), glm::vec3(max.x, max.y, max.z), color, transform);
        s_Instance->AddLine(glm::vec3(min.x, min.y, max.z), glm::vec3(min.x, max.y, max.z), color, transform);
    }

    void DebugPass::DrawSphere(const glm::vec3& center, float radius, int32 segments, const glm::vec4& color, const glm::mat4* transform)
    {
        if (!s_Instance) return;

        const float pi = glm::pi<float>();
        const int32 latSegments = segments / 2;

        // Draw latitude circles
        for (int32 lat = 0; lat < latSegments; ++lat) {
            float theta1 = pi * lat / latSegments;
            float theta2 = pi * (lat + 1) / latSegments;

            for (int32 lon = 0; lon < segments; ++lon) {
                float phi1 = 2.0f * pi * lon / segments;
                float phi2 = 2.0f * pi * (lon + 1) / segments;

                glm::vec3 p1 = center + radius * glm::vec3(
                    sin(theta1) * cos(phi1),
                    cos(theta1),
                    sin(theta1) * sin(phi1)
                );

                glm::vec3 p2 = center + radius * glm::vec3(
                    sin(theta1) * cos(phi2),
                    cos(theta1),
                    sin(theta1) * sin(phi2)
                );

                glm::vec3 p3 = center + radius * glm::vec3(
                    sin(theta2) * cos(phi2),
                    cos(theta2),
                    sin(theta2) * sin(phi2)
                );

                s_Instance->AddLine(p1, p2, color, transform);
                s_Instance->AddLine(p2, p3, color, transform);
            }
        }
    }

    void DebugPass::DrawCapsule(const glm::vec3& from, const glm::vec3& to, float radius, int32 segments, const glm::vec4& color, const glm::mat4* transform)
    {
        if (!s_Instance) return;

        glm::vec3 dir = glm::normalize(to - from);
        glm::vec3 ortho = glm::abs(dir.x) < 0.9f ? glm::cross(dir, glm::vec3(1, 0, 0)) : glm::cross(dir, glm::vec3(0, 1, 0));
        ortho = glm::normalize(ortho);
        glm::vec3 right = glm::cross(dir, ortho);

        const float angleStep = 2.0f * glm::pi<float>() / segments;

        // Draw cylinder sides
        for (int32 i = 0; i < segments; ++i) {
            float angle1 = angleStep * i;
            float angle2 = angleStep * (i + 1);

            glm::vec3 offset1 = radius * (ortho * cosf(angle1) + sinf(angle1) * right);
            glm::vec3 offset2 = radius * (ortho * cosf(angle2) + sinf(angle2) * right);

            s_Instance->AddLine(from + offset1, to + offset1, color, transform);
            s_Instance->AddLine(from + offset1, from + offset2, color, transform);
            s_Instance->AddLine(to + offset1, to + offset2, color, transform);
        }

        // Draw hemisphere caps
        const int32 hemiSegments = segments / 2;
        const float pi = glm::pi<float>();

        auto drawHemi = [&](const glm::vec3& center, const glm::vec3& axisDir) {
            for (int32 i = 0; i < hemiSegments; ++i) {
                float phi1 = pi * 0.5f * i / hemiSegments;
                float phi2 = pi * 0.5f * (i + 1) / hemiSegments;

                for (int32 j = 0; j < segments; ++j) {
                    float theta1 = 2.0f * pi * j / segments;
                    float theta2 = 2.0f * pi * (j + 1) / segments;

                    auto getPos = [&](float p, float t) {
                        return center + radius * (sinf(p) * (cosf(t) * ortho + sinf(t) * right) + cosf(p) * axisDir);
                    };

                    glm::vec3 p1 = getPos(phi1, theta1);
                    glm::vec3 p2 = getPos(phi1, theta2);
                    glm::vec3 p3 = getPos(phi2, theta1);

                    s_Instance->AddLine(p1, p2, color, transform);
                    s_Instance->AddLine(p1, p3, color, transform);
                }
            }
        };

        drawHemi(from, -dir);
        drawHemi(to, dir);
    }

    void DebugPass::DrawCone(const glm::vec3& apex, const glm::vec3& base, float radius, int32 segments, const glm::vec4& color, const glm::mat4* transform)
    {
        if (!s_Instance) return;

        glm::vec3 dir = glm::normalize(base - apex);
        glm::vec3 ortho = glm::abs(dir.x) < 0.9f ? glm::cross(dir, glm::vec3(1, 0, 0)) : glm::cross(dir, glm::vec3(0, 1, 0));
        ortho = glm::normalize(ortho);
        glm::vec3 right = glm::cross(dir, ortho);

        const float angleStep = 2.0f * glm::pi<float>() / segments;

        // Draw base circle
        for (int32 i = 0; i < segments; ++i) {
            float angle1 = angleStep * i;
            float angle2 = angleStep * (i + 1);

            glm::vec3 offset1 = radius * (ortho * cosf(angle1) + sinf(angle1) * right);
            glm::vec3 offset2 = radius * (ortho * cosf(angle2) + sinf(angle2) * right);

            s_Instance->AddLine(base + offset1, base + offset2, color, transform);
            if (i % 4 == 0) {
                s_Instance->AddLine(apex, base + offset1, color, transform);
            }
        }
    }

    void DebugPass::DrawTriangle(const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2, const glm::vec4& color, const glm::mat4* transform)
    {
        if (!s_Instance) return;

        s_Instance->AddLine(p0, p1, color, transform);
        s_Instance->AddLine(p1, p2, color, transform);
        s_Instance->AddLine(p2, p0, color, transform);
    }

    void DebugPass::DrawArrow(const glm::vec3& from, const glm::vec3& to, float headSize, const glm::vec4& color, const glm::mat4* transform)
    {
        if (!s_Instance) return;

        s_Instance->AddLine(from, to, color, transform);

        glm::vec3 dir = glm::normalize(to - from);
        glm::vec3 ortho = glm::abs(dir.x) < 0.9f ? glm::cross(dir, glm::vec3(1, 0, 0)) : glm::cross(dir, glm::vec3(0, 1, 0));
        ortho = glm::normalize(ortho) * headSize;
        glm::vec3 right = glm::normalize(glm::cross(dir, ortho)) * headSize;

        glm::vec3 headBase = to - dir * headSize;

        s_Instance->AddLine(to, headBase + ortho, color, transform);
        s_Instance->AddLine(to, headBase - ortho, color, transform);
        s_Instance->AddLine(to, headBase + right, color, transform);
        s_Instance->AddLine(to, headBase - right, color, transform);
    }

    void DebugPass::DrawPlane(const glm::vec3& center, const glm::vec3& normal, float size, const glm::vec4& color, const glm::mat4* transform)
    {
        if (!s_Instance) return;

        glm::vec3 n = glm::normalize(normal);
        glm::vec3 ortho = glm::abs(n.x) < 0.9f ? glm::cross(n, glm::vec3(1, 0, 0)) : glm::cross(n, glm::vec3(0, 1, 0));
        ortho = glm::normalize(ortho);
        glm::vec3 right = glm::cross(n, ortho);

        float half = size / 2.0f;

        glm::vec3 p0 = center + ortho * half + right * half;
        glm::vec3 p1 = center - ortho * half + right * half;
        glm::vec3 p2 = center - ortho * half - right * half;
        glm::vec3 p3 = center + ortho * half - right * half;

        s_Instance->AddLine(p0, p1, color, transform);
        s_Instance->AddLine(p1, p2, color, transform);
        s_Instance->AddLine(p2, p3, color, transform);
        s_Instance->AddLine(p3, p0, color, transform);

        s_Instance->AddLine(center, center + n * size, color, transform);
    }

    void DebugPass::DrawRing(const glm::vec3& center, const glm::vec3& normal, float radius, int32 segments, const glm::vec4& color, const glm::mat4* transform)
    {
        if (!s_Instance) return;

        glm::vec3 n = glm::normalize(normal);
        glm::vec3 ortho = glm::abs(n.x) < 0.9f ? glm::cross(n, glm::vec3(1, 0, 0)) : glm::cross(n, glm::vec3(0, 1, 0));
        ortho = glm::normalize(ortho);
        glm::vec3 right = glm::cross(n, ortho);

        const float angleStep = 2.0f * glm::pi<float>() / segments;

        for (int32 i = 0; i < segments; ++i) {
            float angle1 = angleStep * i;
            float angle2 = angleStep * (i + 1);

            glm::vec3 p1 = center + radius * (cosf(angle1) * ortho + sinf(angle1) * right);
            glm::vec3 p2 = center + radius * (cosf(angle2) * ortho + sinf(angle2) * right);

            s_Instance->AddLine(p1, p2, color, transform);
        }
    }

    void DebugPass::DrawCylinder(const glm::vec3& from, const glm::vec3& to, float radius, int32 segments, const glm::vec4& color, const glm::mat4* transform)
    {
        if (!s_Instance) return;

        glm::vec3 dir = glm::normalize(to - from);
        glm::vec3 ortho = glm::abs(dir.x) < 0.9f ? glm::cross(dir, glm::vec3(1, 0, 0)) : glm::cross(dir, glm::vec3(0, 1, 0));
        ortho = glm::normalize(ortho);
        glm::vec3 right = glm::cross(dir, ortho);

        const float angleStep = 2.0f * glm::pi<float>() / segments;

        for (int32 i = 0; i < segments; ++i) {
            float angle1 = angleStep * i;
            float angle2 = angleStep * (i + 1);

            glm::vec3 offset1 = radius * (cosf(angle1) * ortho + sinf(angle1) * right);
            glm::vec3 offset2 = radius * (cosf(angle2) * ortho + sinf(angle2) * right);

            // Bottom circle
            s_Instance->AddLine(from + offset1, from + offset2, color, transform);
            // Top circle
            s_Instance->AddLine(to + offset1, to + offset2, color, transform);
            // Vertical lines
            if (i % 4 == 0) {
                s_Instance->AddLine(from + offset1, to + offset1, color, transform);
            }
        }
    }

    void DebugPass::DrawAxes(const glm::vec3& origin, float scale, const glm::mat4* transform)
    {
        if (!s_Instance) return;

        const float length = scale;
        s_Instance->AddLine(origin, origin + glm::vec3(length, 0, 0), glm::vec4(1, 0, 0, 1), transform); // X - Red
        s_Instance->AddLine(origin, origin + glm::vec3(0, length, 0), glm::vec4(0, 1, 0, 1), transform); // Y - Green
        s_Instance->AddLine(origin, origin + glm::vec3(0, 0, length), glm::vec4(0, 0, 1, 1), transform); // Z - Blue
    }

    void DebugPass::DrawArc(const glm::vec3& center, const glm::vec3& normal, const glm::vec3& startDir, float radius, float angle, int32 segments, const glm::vec4& color, const glm::mat4* transform)
    {
        if (!s_Instance) return;

        glm::vec3 n = glm::normalize(normal);
        glm::vec3 start = glm::normalize(startDir);

        const float angleStep = angle / segments;

        for (int32 i = 0; i < segments; ++i) {
            float a1 = angleStep * i;
            float a2 = angleStep * (i + 1);

            // Rodrigues' rotation formula to rotate start direction around normal
            glm::vec3 p1 = center + radius * (
                cosf(a1) * start +
                sinf(a1) * glm::cross(n, start)
            );
            glm::vec3 p2 = center + radius * (
                cosf(a2) * start +
                sinf(a2) * glm::cross(n, start)
            );

            s_Instance->AddLine(p1, p2, color, transform);
        }
    }

    void DebugPass::DrawGrid(const glm::vec3& center, const glm::vec3& normal, float size, int32 cells, const glm::vec4& color)
    {
        if (!s_Instance) return;

        glm::vec3 n = glm::normalize(normal);
        glm::vec3 ortho = glm::abs(n.x) < 0.9f ? glm::cross(n, glm::vec3(1, 0, 0)) : glm::cross(n, glm::vec3(0, 1, 0));
        ortho = glm::normalize(ortho);
        glm::vec3 right = glm::cross(n, ortho);

        float halfSize = size * 0.5f;
        float step = size / static_cast<float>(cells);

        for (int32 i = 0; i <= cells; ++i)
        {
            float offset = -halfSize + static_cast<float>(i) * step;

            // Lines parallel to 'right'
            s_Instance->AddLine(
                center + ortho * offset - right * halfSize,
                center + ortho * offset + right * halfSize,
                color
            );

            // Lines parallel to 'ortho'
            s_Instance->AddLine(
                center + right * offset - ortho * halfSize,
                center + right * offset + ortho * halfSize,
                color
            );
        }
    }



    void DebugPass::DrawFrustum(const glm::mat4& viewProj, const glm::vec4& color)
    {
        if (!s_Instance) return;

        glm::mat4 invVP = glm::inverse(viewProj);
        glm::vec4 corners[8] = {
            { -1.0f, -1.0f,  0.0f, 1.0f }, {  1.0f, -1.0f,  0.0f, 1.0f },
            {  1.0f,  1.0f,  0.0f, 1.0f }, { -1.0f,  1.0f,  0.0f, 1.0f },
            { -1.0f, -1.0f,  1.0f, 1.0f }, {  1.0f, -1.0f,  1.0f, 1.0f },
            {  1.0f,  1.0f,  1.0f, 1.0f }, { -1.0f,  1.0f,  1.0f, 1.0f }
        };

        glm::vec3 worldCorners[8];
        for (int i = 0; i < 8; ++i) {
            glm::vec4 worldPos = invVP * corners[i];
            worldCorners[i] = glm::vec3(worldPos) / worldPos.w;
        }

        // Near plane
        s_Instance->AddLine(worldCorners[0], worldCorners[1], color);
        s_Instance->AddLine(worldCorners[1], worldCorners[2], color);
        s_Instance->AddLine(worldCorners[2], worldCorners[3], color);
        s_Instance->AddLine(worldCorners[3], worldCorners[0], color);

        // Far plane
        s_Instance->AddLine(worldCorners[4], worldCorners[5], color);
        s_Instance->AddLine(worldCorners[5], worldCorners[6], color);
        s_Instance->AddLine(worldCorners[6], worldCorners[7], color);
        s_Instance->AddLine(worldCorners[7], worldCorners[4], color);

        // Connecting lines
        s_Instance->AddLine(worldCorners[0], worldCorners[4], color);
        s_Instance->AddLine(worldCorners[1], worldCorners[5], color);
        s_Instance->AddLine(worldCorners[2], worldCorners[6], color);
        s_Instance->AddLine(worldCorners[3], worldCorners[7], color);
    }

    void DebugPass::Clear()
    {
        s_Vertices.clear();
    }

    DebugPass* DebugPass::GetInstance()
    {
        return s_Instance;
    }

}
