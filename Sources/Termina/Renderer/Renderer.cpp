#include "Renderer.hpp"

#include "Core/Application.hpp"
#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_glfw.h"
#include "Renderer/Passes/CubesPass.hpp"
#include "Renderer/Passes/ImGuiPass.hpp"
#include "Renderer/Passes/TrianglePass.hpp"
#include "Renderer/UIUtils.hpp"
#include "World/WorldSystem.hpp"

namespace Termina {
    RendererSystem::RendererSystem(Window* window)
        : m_Window(window)
    {
        m_Device = RendererDevice::Create();
        m_Surface = m_Device->CreateSurface(window);

        m_CurrentWidth = window->GetWidth();
        m_CurrentHeight = window->GetHeight();

        m_GPUAllocator = new GPUBumpAllocator(m_Device, 1024 * 1024 * 16); // 16 MB
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

    void RendererSystem::BakeTimeline()
    {
        m_RenderPasses = {
            new CubesPass(),
            new ImGuiPass()
        };
    }
}
