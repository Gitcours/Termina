#include "Renderer.hpp"

#include "Core/Application.hpp"
#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_glfw.h"
#include "Renderer/Components/CameraComponent.hpp"
#include "Renderer/Components/MeshComponent.hpp"
#include "Renderer/Components/DirectionalLightComponent.hpp"
#include "Renderer/Components/PointLightComponent.hpp"
#include "Renderer/Components/SpotLightComponent.hpp"
#include "Renderer/Passes/CubesPass.hpp"
#include "Renderer/Passes/GBufferPass.hpp"
#include "Renderer/Passes/ShadowPass.hpp"
#include "Renderer/Passes/DeferredPass.hpp"
#include "Renderer/Passes/TonemapPass.hpp"
#include "Renderer/Passes/ImGuiPass.hpp"
#include "Renderer/Passes/DebugPass.hpp"
#include "Renderer/Passes/FXAAPass.hpp"
#include "Renderer/Passes/SkyPass.hpp"
#include "Renderer/UIUtils.hpp"
#include "World/Component.hpp"
#include "World/ComponentRegistry.hpp"
#include "World/WorldSystem.hpp"

namespace Termina {
    RendererSystem::RendererSystem(Window* window)
        : m_Window(window)
    {
        m_Device = RendererDevice::Create();
        m_Surface = m_Device->CreateSurface(window);

        m_CurrentWidth = window->GetWidth();
        m_CurrentHeight = window->GetHeight();

        m_GPUAllocator = new GPUBumpAllocator(m_Device, 1024 * 512); // 512 KB
        m_TemporaryContext = new TemporaryContext(m_Device);
        m_GPUUploader = new GPUUploader(m_Device, FRAMES_IN_FLIGHT);
        m_ResourceViewCache = new ResourceViewCache(m_Device);
        m_SamplerCache = new SamplerCache(m_Device);

        // Initialize ImGui
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui::StyleColorsLight();

        ImGuiIO& io = ImGui::GetIO();
        io.BackendRendererName = "Termina ImGui Renderer";
        io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
        io.BackendFlags |= ImGuiBackendFlags_RendererHasTextures;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

        ImGui_ImplGlfw_InitForVulkan(window->GetHandle(), true);

        UIUtils::Setup();
    }

    RendererSystem::~RendererSystem()
    {
        m_Device->WaitIdle();
        m_PassIO.Clear();

        for (RenderPass* pass : m_RenderPasses) {
            delete pass;
        }
        delete m_GPUAllocator;
        delete m_TemporaryContext;
        delete m_GPUUploader;
        delete m_ResourceViewCache;
        delete m_SamplerCache;
        delete m_Surface;
        delete m_Device;
    }

    void RendererSystem::PreUpdate(float deltaTime)
    {
        ImGuiIO& io = ImGui::GetIO();
        io.DisplaySize.x = (float)m_Window->GetWidth();
        io.DisplaySize.y = (float)m_Window->GetHeight();

        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
    }

    void RendererSystem::PreRender(float deltaTime)
    {
        if (!m_BakedTimeline) {
            BakeTimeline();
            m_BakedTimeline = true;
        }
        m_PendingLights.clear();
    }

    void RendererSystem::Render(float deltaTime)
    {
        if (m_Window->GetWidth() != m_CurrentWidth || m_Window->GetHeight() != m_CurrentHeight) {
            m_Device->WaitIdle();
            m_CurrentWidth = m_Window->GetWidth();
            m_CurrentHeight = m_Window->GetHeight();

            m_Surface->Resize(m_Window->GetWidth(), m_Window->GetHeight());
            for (auto& pass : m_RenderPasses) {
                pass->Resize(m_CurrentWidth, m_CurrentHeight);
            }
        }

        int32 pixelWidth = m_Window->GetPixelWidth();
        int32 pixelHeight = m_Window->GetPixelHeight();

        uint32 frameIndex = m_Surface->GetFrameIndex();

        RenderContext* context = m_Surface->BeginFrame();
        m_GPUUploader->BeginFrame(frameIndex);
        m_GPUUploader->RecordUploads(context);
        m_GPUAllocator->Reset();

        for (RenderPass* pass : m_RenderPasses) {
            RenderPassExecuteInfo info = {
                .Device = m_Device,
                .Surface = m_Surface,
                .Ctx = context,

                .Uploader = m_GPUUploader,
                .Allocator = m_GPUAllocator,
                .ViewCache = m_ResourceViewCache,
                .SampCache = m_SamplerCache,
                .IO = &m_PassIO,

                .CurrentWorld = Application::GetSystem<WorldSystem>()->GetCurrentWorld(),
                .CurrentCamera = m_CurrentCamera,
                .LightList = &m_PendingLights,

                .FrameIndex = frameIndex,
                .Width = pixelWidth,
                .Height = pixelHeight
            };
            pass->Execute(info);
        }
        for (const auto& callback : m_RenderCallbacks) {
            callback(m_Device, m_Surface, deltaTime);
        }
        m_Surface->EndFrame();
    }

    void RendererSystem::RegisterComponents()
    {
        ComponentRegistry::Get().Register<CameraComponent>("Camera Component");
        ComponentRegistry::Get().Register<MeshComponent>("Mesh Component");
        ComponentRegistry::Get().Register<DirectionalLightComponent>("Directional Light");
        ComponentRegistry::Get().Register<PointLightComponent>("Point Light");
        ComponentRegistry::Get().Register<SpotLightComponent>("Spot Light");
    }

    void RendererSystem::UnregisterComponents()
    {
        ComponentRegistry::Get().UnregisterByName("Camera Component");
        ComponentRegistry::Get().UnregisterByName("Mesh Component");
        ComponentRegistry::Get().UnregisterByName("Directional Light");
        ComponentRegistry::Get().UnregisterByName("Point Light");
        ComponentRegistry::Get().UnregisterByName("Spot Light");
    }

    void RendererSystem::BakeTimeline()
    {
        m_RenderPasses = {
            new GBufferPass(),
            new ShadowPass(),
            new DeferredPass(),
            new SkyPass(),
            new TonemapPass(),
            new FXAAPass(),
            new DebugPass(),
            new ImGuiPass(m_ShouldImGuiClear)
        };
    }

    void RendererSystem::ShowDebugWindow(bool* open)
    {
        if (!ImGui::Begin("Renderer", open))
        {
            ImGui::End();
            return;
        }

        // PassIO
        if (ImGui::CollapsingHeader("PassIO"))
        {
            auto texNames = m_PassIO.GetTextureNames();
            auto bufNames = m_PassIO.GetBufferNames();

            ImGui::Text("Textures: %zu", texNames.size());
            for (auto& name : texNames)
            {
                ImGui::BulletText("%s", name.c_str());
            }

            ImGui::Text("Buffers: %zu", bufNames.size());
            for (auto& name : bufNames)
            {
                ImGui::BulletText("%s", name.c_str());
            }
        }

        // Resource view cache
        if (ImGui::CollapsingHeader("Resource View Cache"))
        {
            ImGui::Text("Cached views: %zu", m_ResourceViewCache ? m_ResourceViewCache->GetViewCount() : 0);
        }

        // Sampler cache
        if (ImGui::CollapsingHeader("Sampler Cache"))
        {
            ImGui::Text("Cached samplers: %zu", m_SamplerCache ? m_SamplerCache->GetSamplerCount() : 0);
        }

        // Render passes
        if (ImGui::CollapsingHeader("Render Passes"))
        {
            ImGui::Text("Passes: %zu", m_RenderPasses.size());
            for (size_t i = 0; i < m_RenderPasses.size(); ++i)
            {
                ImGui::PushID(static_cast<int>(i));
                if (ImGui::TreeNode("pass", "[%zu] %p", i, static_cast<void*>(m_RenderPasses[i])))
                {
                    m_RenderPasses[i]->Inspect();
                    ImGui::TreePop();
                }
                ImGui::PopID();
            }
        }

        ImGui::End();
    }
}

    void* Termina::RendererSystem::GetImGuiContext() {
        return ImGui::GetCurrentContext();
    }

    void Termina::RendererSystem::GetImGuiAllocator(void** allocFunc, void** freeFunc, void** userData) {
        ImGui::GetAllocatorFunctions((ImGuiMemAllocFunc*)allocFunc, (ImGuiMemFreeFunc*)freeFunc, userData);
    }
