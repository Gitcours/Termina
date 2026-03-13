#include "ShaderServer.hpp"
#include "ShaderFile.hpp"

#include <Termina/Core/Logger.hpp>
#include <string>

namespace Termina {
    ShaderServer::ShaderServer()
        : m_Device(nullptr)
    {
        m_Shaders = {};
    }

    void ShaderServer::Clear()
    {
        // Delete all pending deletions first
        for (auto* pipeline : m_PendingDeletion) {
            if (pipeline) delete pipeline;
        }
        m_PendingDeletion.clear();

        // Then delete all active pipelines
        for (auto& [path, entry] : m_Shaders) {
            for (auto& [variantKey, pipeline] : entry.GraphicsPipelines) {
                if (pipeline) delete pipeline;
            }
            entry.GraphicsPipelines.clear();

            for (auto& [variantKey, pipeline] : entry.ComputePipelines) {
                if (pipeline) delete pipeline;
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
            TN_ERROR("Shader file %s doesn't exist!", path.c_str());
            return;
        }

        ShaderEntry entry;
        entry.Type = type;
        entry.GraphicsDesc = desc;
        entry.FileHandle = FileSystem::WatchFile(path);
        if (entry.GraphicsPipelines.empty()) {
            ShaderFile shaderFile;
            if (!shaderFile.Load(path)) {
                TN_ERROR("Failed to reload shader file %s", path.c_str());
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
                if (variantEntry.Bytecodes.count(ShaderType::GEOMETRY) > 0) {
                    ShaderModule module;
                    module.EntryPoint = variantEntry.Bytecodes[ShaderType::GEOMETRY].first;
                    module.Bytecode = variantEntry.Bytecodes[ShaderType::GEOMETRY].second;
                    module.Type = ShaderType::GEOMETRY;

                    entry.GraphicsDesc.SetShaderBytecode(ShaderType::GEOMETRY, module.Bytecode, module.EntryPoint);
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
            TN_ERROR("Shader file %s doesn't exist!", path.c_str());
            return;
        }

        ShaderEntry entry;
        entry.Type = type;
        entry.FileHandle = FileSystem::WatchFile(path);
        if (entry.ComputePipelines.empty()) {
            ShaderFile shaderFile;
            if (!shaderFile.Load(path)) {
                TN_ERROR("Failed to reload shader file %s", path.c_str());
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
            m_Device->WaitIdle();

            for (auto* pipeline : m_PendingDeletion) {
                if (pipeline) delete pipeline;
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
                    TN_ERROR("Failed to reload shader file %s", path.c_str());
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
                        if (variantEntry.Bytecodes.count(ShaderType::GEOMETRY) > 0) {
                            ShaderModule module;
                            module.EntryPoint = variantEntry.Bytecodes[ShaderType::GEOMETRY].first;
                            module.Bytecode = variantEntry.Bytecodes[ShaderType::GEOMETRY].second;
                            module.Type = ShaderType::GEOMETRY;

                            entry.GraphicsDesc.SetShaderBytecode(ShaderType::GEOMETRY, module.Bytecode, module.EntryPoint);
                        }

                        RenderPipeline* pipeline = m_Device->CreateRenderPipeline(entry.GraphicsDesc);
                        if (!pipeline) {
                            TN_ERROR("Failed to create pipeline for variant %s", variantKey.c_str());
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
                        TN_INFO("Successfully reloaded shader: %s", path.c_str());
                    } else {
                        // Clean up failed pipelines
                        for (auto& [variantKey, pipeline] : newGraphicsPipelines) {
                            if (pipeline) delete pipeline;
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
                                TN_ERROR("Failed to create compute pipeline for variant %s", variantKey.c_str());
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
                        TN_INFO("Successfully reloaded compute shader: %s", path.c_str());
                    } else {
                        // Clean up failed pipelines
                        for (auto& [variantKey, pipeline] : newComputePipelines) {
                            if (pipeline) delete pipeline;
                        }
                        entry.FileHandle = FileSystem::WatchFile(path); // Update timestamp to avoid retry spam
                    }
                }
            }
        }

        // TODO: Mesh pipeline
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
}
