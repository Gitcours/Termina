#include "ShaderServer.h"
#include "Engine/Core/Logger.h"
#include "Engine/Renderer/Backends/RenderPipeline.h"
#include "ShaderFile.h"

#include "Engine/Core/Memory/Memory.h"

#include "ImGui/imgui.h"
#include <string>

ShaderServer::ShaderServer()
    : m_Device(nullptr)
{
}

void ShaderServer::Clear()
{
    if (m_Device) {
        m_Device->WaitIdle();
    }
    // Delete all pending deletions first
    for (auto* pipeline : m_PendingDeletion) {
        if (pipeline) MEMORY_DELETE(Renderer, pipeline);
    }
    m_PendingDeletion.clear();

    // Then delete all active pipelines
    for (auto& [path, entry] : m_Shaders) {
        for (auto& [variantKey, pipeline] : entry.GraphicsPipelines) {
            if (pipeline) MEMORY_DELETE(Renderer, pipeline);
        }
        entry.GraphicsPipelines.clear();
        
        for (auto& [variantKey, pipeline] : entry.ComputePipelines) {
            if (pipeline) MEMORY_DELETE(Renderer, pipeline);
        }
        entry.ComputePipelines.clear();
    }
    m_Shaders.clear();
}

void ShaderServer::ConnectDevice(RendererDevice* device)
{
    m_Device = device;
}

void ShaderServer::WatchPipeline(const std::string& path, const RenderPipelineDesc& desc, PipelineType type)
{
    if (!FileSystem::FileExists(path)) {
        LOG_ERROR("Shader file %s doesn't exist!", path.c_str());
        return;
    }

    ShaderEntry entry;
    entry.Type = type;
    entry.GraphicsDesc = desc;
    entry.FileHandle = FileSystem::WatchFile(path);
    if (entry.GraphicsPipelines.empty()) {
        ShaderFile shaderFile;
        if (!shaderFile.Load(path)) {
            LOG_ERROR("Failed to reload shader file %s", path.c_str());
            return;
        }

        // Note: entry.GraphicsPipelines is already empty here in practice
        // But if they exist, defer deletion
        for (auto& [variantKey, pipeline] : entry.GraphicsPipelines) {
            if (pipeline) {
                m_PendingDeletion.push_back(pipeline);
            }
        }
        entry.GraphicsPipelines.clear();
        entry.FileHandle = FileSystem::WatchFile(path);

        for (auto& [variantKey, variantEntry] : shaderFile.Variants) {
            if (variantEntry.Bytecodes.count(ShaderType::VERTEX) > 0) {
                ShaderModule module;
                module.EntryPoint = variantEntry.Bytecodes[ShaderType::VERTEX].first;
                module.Bytecode = variantEntry.Bytecodes[ShaderType::VERTEX].second;
                module.Type = ShaderType::VERTEX;

                entry.GraphicsDesc.SetShaderBytecode(ShaderType::VERTEX, module.Bytecode, module.EntryPoint);
            }
            if (variantEntry.Bytecodes.count(ShaderType::PIXEL) > 0) {
                ShaderModule module;
                module.EntryPoint = variantEntry.Bytecodes[ShaderType::PIXEL].first;
                module.Bytecode = variantEntry.Bytecodes[ShaderType::PIXEL].second;
                module.Type = ShaderType::PIXEL;

                entry.GraphicsDesc.SetShaderBytecode(ShaderType::PIXEL, module.Bytecode, module.EntryPoint);
            }
            entry.GraphicsDesc.Name = path + " [RENDER: " + variantKey + "]";

            RenderPipeline* pipeline = m_Device->CreateRenderPipeline(entry.GraphicsDesc);
            entry.GraphicsPipelines[variantKey] = pipeline;
        }
    }

    m_Shaders[path] = entry;
}

void ShaderServer::WatchPipeline(const std::string& path, PipelineType type)
{
    if (!FileSystem::FileExists(path)) {
        LOG_ERROR("Shader file %s doesn't exist!", path.c_str());
        return;
    }

    ShaderEntry entry;
    entry.Type = type;
    entry.FileHandle = FileSystem::WatchFile(path);
    if (entry.ComputePipelines.empty()) {
        ShaderFile shaderFile;
        if (!shaderFile.Load(path)) {
            LOG_ERROR("Failed to reload shader file %s", path.c_str());
            return;
        }

        for (auto& [variantKey, pipeline] : entry.ComputePipelines) {
            if (pipeline) {
                m_PendingDeletion.push_back(pipeline);
            }
        }
        entry.ComputePipelines.clear();
        entry.FileHandle = FileSystem::WatchFile(path);

        for (auto& [variantKey, variantEntry] : shaderFile.Variants) {
            ShaderModule module;
            module.EntryPoint = variantEntry.Bytecodes[ShaderType::COMPUTE].first;
            module.Bytecode = variantEntry.Bytecodes[ShaderType::COMPUTE].second;
            module.Type = ShaderType::COMPUTE;

            std::string name = path + " [RENDER: " + variantKey + "]";
            ComputePipeline* pipeline = m_Device->CreateComputePipeline(module, path);
            entry.ComputePipelines[variantKey] = pipeline;
        }
    }
    
    m_Shaders[path] = entry;
}

void ShaderServer::ProcessPendingDeletions()
{
    if (!m_PendingDeletion.empty()) {
        // Wait for GPU to finish before deleting old GraphicsPipelines
        m_Device->WaitIdle();

        for (auto* pipeline : m_PendingDeletion) {
            if (pipeline) MEMORY_DELETE(Renderer, pipeline);
        }
        m_PendingDeletion.clear();
    }
}

void ShaderServer::ReloadModifiedPipelines()
{
    // Delete pipelines from previous frame that are no longer in use
    ProcessPendingDeletions();

    for (auto& [path, entry] : m_Shaders) {
        if (FileSystem::HasFileChanged(entry.FileHandle)) {
            // Load shader file first
            ShaderFile shaderFile;
            if (!shaderFile.Load(path)) {
                LOG_ERROR("Failed to reload shader file %s", path.c_str());
                entry.FileHandle = FileSystem::WatchFile(path); // Update timestamp to avoid retry spam
                continue; // Keep old pipelines if reload fails
            }

            bool success = true;

            if (entry.Type == PipelineType::Graphics) {
                // Create new graphics pipelines in temporary map
                std::unordered_map<std::string, RenderPipeline*> newGraphicsPipelines;

                for (auto& [variantKey, variantEntry] : shaderFile.Variants) {
                    if (variantEntry.Bytecodes.count(ShaderType::VERTEX) > 0) {
                        ShaderModule module;
                        module.EntryPoint = variantEntry.Bytecodes[ShaderType::VERTEX].first;
                        module.Bytecode = variantEntry.Bytecodes[ShaderType::VERTEX].second;
                        module.Type = ShaderType::VERTEX;

                        entry.GraphicsDesc.SetShaderBytecode(ShaderType::VERTEX, module.Bytecode, module.EntryPoint);
                    }
                    if (variantEntry.Bytecodes.count(ShaderType::PIXEL) > 0) {
                        ShaderModule module;
                        module.EntryPoint = variantEntry.Bytecodes[ShaderType::PIXEL].first;
                        module.Bytecode = variantEntry.Bytecodes[ShaderType::PIXEL].second;
                        module.Type = ShaderType::PIXEL;

                        entry.GraphicsDesc.SetShaderBytecode(ShaderType::PIXEL, module.Bytecode, module.EntryPoint);
                    }

                    RenderPipeline* pipeline = m_Device->CreateRenderPipeline(entry.GraphicsDesc);
                    if (!pipeline) {
                        LOG_ERROR("Failed to create pipeline for variant %s", variantKey.c_str());
                        success = false;
                        break;
                    }
                    newGraphicsPipelines[variantKey] = pipeline;
                }

                // Only swap if all pipelines created successfully
                if (success && !newGraphicsPipelines.empty()) {
                    // Mark old pipelines for deletion next frame
                    for (auto& [variantKey, pipeline] : entry.GraphicsPipelines) {
                        if (pipeline) {
                            m_PendingDeletion.push_back(pipeline);
                        }
                    }
                    // Swap in new pipelines
                    entry.GraphicsPipelines = std::move(newGraphicsPipelines);
                    entry.FileHandle = FileSystem::WatchFile(path);
                    LOG_INFO("Successfully reloaded shader: %s", path.c_str());
                } else {
                    // Clean up failed pipelines
                    for (auto& [variantKey, pipeline] : newGraphicsPipelines) {
                        if (pipeline) MEMORY_DELETE(Renderer, pipeline);
                    }
                    entry.FileHandle = FileSystem::WatchFile(path); // Update timestamp to avoid retry spam
                }
            } else if (entry.Type == PipelineType::Compute) {
                // Create new compute pipelines in temporary map
                std::unordered_map<std::string, ComputePipeline*> newComputePipelines;

                for (auto& [variantKey, variantEntry] : shaderFile.Variants) {
                    if (variantEntry.Bytecodes.count(ShaderType::COMPUTE) > 0) {
                        ShaderModule module;
                        module.EntryPoint = variantEntry.Bytecodes[ShaderType::COMPUTE].first;
                        module.Bytecode = variantEntry.Bytecodes[ShaderType::COMPUTE].second;
                        module.Type = ShaderType::COMPUTE;

                        std::string name = path + " [COMPUTE: " + variantKey + "]";
                        ComputePipeline* pipeline = m_Device->CreateComputePipeline(module, name);
                        if (!pipeline) {
                            LOG_ERROR("Failed to create compute pipeline for variant %s", variantKey.c_str());
                            success = false;
                            break;
                        }
                        newComputePipelines[variantKey] = pipeline;
                    }
                }

                // Only swap if all pipelines created successfully
                if (success && !newComputePipelines.empty()) {
                    // Mark old pipelines for deletion next frame
                    for (auto& [variantKey, pipeline] : entry.ComputePipelines) {
                        if (pipeline) {
                            m_PendingDeletion.push_back(pipeline);
                        }
                    }
                    // Swap in new pipelines
                    entry.ComputePipelines = std::move(newComputePipelines);
                    entry.FileHandle = FileSystem::WatchFile(path);
                    LOG_INFO("Successfully reloaded compute shader: %s", path.c_str());
                } else {
                    // Clean up failed pipelines
                    for (auto& [variantKey, pipeline] : newComputePipelines) {
                        if (pipeline) MEMORY_DELETE(Renderer, pipeline);
                    }
                    entry.FileHandle = FileSystem::WatchFile(path); // Update timestamp to avoid retry spam
                }
            }
        }
    }
}

RenderPipeline* ShaderServer::GetPipeline(const std::string& path, const std::vector<std::string>& defines)
{
    if (m_Shaders.count(path) == 0) {
        return nullptr;
    }

    ShaderEntry& entry = m_Shaders[path];
    std::string variantKey;
    for (const auto& define : defines) {
        if (!variantKey.empty()) variantKey += "_";
        variantKey += define;
    }
    return entry.GraphicsPipelines[variantKey];
}

ComputePipeline* ShaderServer::GetComputePipeline(const std::string& path, const std::vector<std::string>& defines)
{
    if (m_Shaders.count(path) == 0) {
        return nullptr;
    }

    ShaderEntry& entry = m_Shaders[path];
    std::string variantKey;
    for (const auto& define : defines) {
        if (!variantKey.empty()) variantKey += "_";
        variantKey += define;
    }
    return entry.ComputePipelines[variantKey];
}

void ShaderServer::RenderDebugUI()
{
    // Helper lambda for formatting sizes
    auto formatSize = [](uint64 bytes) -> std::string {
        if (bytes > (1024 * 1024)) {
            return std::to_string(static_cast<float>(bytes) / (1024.0f * 1024.0f)).substr(0, 5) + " MB";
        } else if (bytes > 1024) {
            return std::to_string(static_cast<float>(bytes) / 1024.0f).substr(0, 5) + " KB";
        } else {
            return std::to_string(bytes) + " B";
        }
    };

    // Top controls
    ImGui::SetNextItemWidth(300.0f);
    ImGui::InputTextWithHint("##ShaderSearch", "Search shaders...", m_SearchFilter, sizeof(m_SearchFilter));
    ImGui::SameLine();

    if (ImGui::Button("Expand All")) {
        m_ExpandAll = true;
        m_CollapseAll = false;
    }
    ImGui::SameLine();
    if (ImGui::Button("Collapse All")) {
        m_CollapseAll = true;
        m_ExpandAll = false;
    }

    ImGui::Separator();

    // Calculate totals
    uint64 totalSize = 0;
    uint64 totalGraphicsPipelines = 0;
    uint64 totalComputePipelines = 0;
    for (auto& [path, entry] : m_Shaders) {
        for (auto& [variantKey, pipeline] : entry.GraphicsPipelines) {
            totalSize += pipeline->GetSize();
            totalGraphicsPipelines++;
        }
        for (auto& [variantKey, pipeline] : entry.ComputePipelines) {
            totalSize += pipeline->GetSize();
            totalComputePipelines++;
        }
    }

    // Per-shader list
    for (auto& [path, entry] : m_Shaders) {
        // Check filter
        if (m_SearchFilter[0] != '\0' && path.find(m_SearchFilter) == std::string::npos) {
            continue;
        }

        // Calculate per-shader stats
        uint64 shaderSize = 0;
        uint64 variantCount = 0;
        for (auto& [variantKey, pipeline] : entry.GraphicsPipelines) {
            shaderSize += pipeline->GetSize();
            variantCount++;
        }
        for (auto& [variantKey, pipeline] : entry.ComputePipelines) {
            shaderSize += pipeline->GetSize();
            variantCount++;
        }

        // Tree node with better formatting
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_None;
        if (m_ExpandAll) flags |= ImGuiTreeNodeFlags_DefaultOpen;

        ImVec4 typeColor = entry.Type == PipelineType::Graphics ? ImVec4(0.2f, 0.8f, 0.2f, 1.0f) : ImVec4(0.2f, 0.6f, 0.9f, 1.0f);
        ImGui::PushStyleColor(ImGuiCol_Text, typeColor);

        std::string treeLabel = std::string(entry.Type == PipelineType::Graphics ? "[G]" : "[C]") + " " + path
            + " (" + std::to_string(variantCount) + " variants) - " + formatSize(shaderSize);

        bool nodeOpen = ImGui::TreeNodeEx(treeLabel.c_str(), flags);
        ImGui::PopStyleColor();

        if (nodeOpen) {
            ImGui::Text("File: %s", entry.FileHandle.Path.c_str());
            ImGui::Text("Type: %s", entry.Type == PipelineType::Graphics ? "Graphics" : "Compute");

            // Memory bar visualization
            float barWidth = 300.0f;
            float shaderRatio = static_cast<float>(shaderSize) / static_cast<float>(totalSize + 1);
            ImGui::ProgressBar(shaderRatio, ImVec2(barWidth, 20.0f), formatSize(shaderSize).c_str());

            ImGui::Separator();
            ImGui::Text("Variants (%llu):", variantCount);
            ImGui::Indent();

            // Display graphics pipeline variants
            for (auto& [variantKey, pipeline] : entry.GraphicsPipelines) {
                uint64 pipelineSize = pipeline->GetSize();

                // Warn if pipeline is large (over 1MB)
                ImVec4 sizeColor = pipelineSize > (1024 * 1024) ? ImVec4(1.0f, 0.5f, 0.0f, 1.0f) : ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
                ImGui::PushStyleColor(ImGuiCol_Text, sizeColor);

                if (ImGui::TreeNode(variantKey.c_str())) {
                    ImGui::Text("Name: %s", pipeline->GetName().c_str());
                    ImGui::Text("Type: Graphics");
                    ImGui::Text("Size: %s", formatSize(pipelineSize).c_str());
                    if (pipelineSize > (1024 * 1024)) {
                        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "⚠ Large pipeline!");
                    }
                    ImGui::TreePop();
                }
                ImGui::PopStyleColor();
            }

            // Display compute pipeline variants
            for (auto& [variantKey, pipeline] : entry.ComputePipelines) {
                uint64 pipelineSize = pipeline->GetSize();

                // Warn if pipeline is large (over 1MB)
                ImVec4 sizeColor = pipelineSize > (1024 * 1024) ? ImVec4(1.0f, 0.5f, 0.0f, 1.0f) : ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
                ImGui::PushStyleColor(ImGuiCol_Text, sizeColor);

                if (ImGui::TreeNode(variantKey.c_str())) {
                    ImGui::Text("Name: %s", pipeline->GetName().c_str());
                    ImGui::Text("Type: Compute");
                    ImGui::Text("Size: %s", formatSize(pipelineSize).c_str());
                    if (pipelineSize > (1024 * 1024)) {
                        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "⚠ Large pipeline!");
                    }
                    ImGui::TreePop();
                }
                ImGui::PopStyleColor();
            }

            ImGui::Unindent();
            ImGui::TreePop();
        }
    }

    ImGui::Separator();
    ImGui::Spacing();

    // Summary stats with better formatting
    ImGui::Text("=== SUMMARY ===");
    ImGui::Text("Total Memory: %s", formatSize(totalSize).c_str());
    ImGui::Text("Total Graphics Pipelines: %llu", totalGraphicsPipelines);
    ImGui::Text("Total Compute Pipelines: %llu", totalComputePipelines);
    ImGui::Text("Total Shaders: %llu", static_cast<uint64>(m_Shaders.size()));

    // Progress bar for total usage
    float maxMemory = 100.0f * 1024.0f * 1024.0f; // 100MB reference
    ImGui::ProgressBar(std::min(1.0f, static_cast<float>(totalSize) / maxMemory), ImVec2(-1.0f, 20.0f), "");
    ImGui::SameLine();
    ImGui::Text("(out of ~100MB reference)");

    // Reset expand/collapse state
    if (m_ExpandAll || m_CollapseAll) {
        m_ExpandAll = false;
        m_CollapseAll = false;
    }
}
