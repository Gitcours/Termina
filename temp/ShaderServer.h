#pragma once

#include "Engine/Core/FileSystem.h"

#include "Engine/Renderer/Backends/Pipeline.h"
#include "Engine/Renderer/Backends/RenderPipeline.h"
#include "Engine/Renderer/Backends/ComputePipeline.h"

#include "Engine/Renderer/Backends/Device.h"
#include <unordered_map>

class ShaderServer
{
public:
    ShaderServer();
    ~ShaderServer() = default;

    void Clear();

    void ConnectDevice(RendererDevice* device);
    void WatchPipeline(const std::string& path, const RenderPipelineDesc& desc, PipelineType type);
    void WatchPipeline(const std::string& path, PipelineType type);

    RenderPipeline* GetPipeline(const std::string& path, const std::vector<std::string>& defines = {"DEFAULT"});
    ComputePipeline* GetComputePipeline(const std::string& path, const std::vector<std::string>& defines = {"DEFAULT"});

    void ReloadModifiedPipelines();
    void ProcessPendingDeletions(); // Call at start of frame
    void RenderDebugUI();
private:
    struct ShaderEntry
    {
        PipelineType Type;

        std::unordered_map<std::string, RenderPipeline*> GraphicsPipelines;
        RenderPipelineDesc GraphicsDesc;

        std::unordered_map<std::string, ComputePipeline*> ComputePipelines;
        
        FileSystem::Watch FileHandle;
    };

    RendererDevice* m_Device;
    std::unordered_map<std::string, ShaderEntry> m_Shaders;
    std::vector<Pipeline*> m_PendingDeletion; // Pipelines to delete next frame

    char m_SearchFilter[256] = {};
    bool m_ExpandAll = false;
    bool m_CollapseAll = false;
};
