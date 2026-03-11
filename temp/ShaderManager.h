#pragma once

#include "Engine/Core/Subsystem/Subsystem.h"
#include "Engine/Platform/SharedLibrary.h"

#include "ShaderServer.h"

#include <string>

/*
    Example shader in Content/Shaders/Example.hlsl

    Shader compilation process:
    - Preprocessing of permutations, includes and macros (ShaderPreprocessor) -- DONE
    - DXC Compilation (HLSL -> DXIL for Windows/MacOS, SPIR-V for Linux) (ShaderCompiler) -- DONE
    - Metal Shader Converter (macOS only, DXIL -> MSL) (MetalShaderConverter) -- TODO
    - Caching, hot reload, management (ShaderServer)

    Shader Manager manages everything, and can also report ShaderStatistics (like number of shaders, permutations, cache size in bytes etc.)
*/

class ShaderManager : public Subsystem
{
public:
    ShaderManager() = default;
	~ShaderManager() = default;

    bool Initialize() override;
	void Shutdown() override;
    void PostRender() override;

	const char* GetName() const override { return "Shader Manager"; }
    ShaderServer& GetShaderServer() { return m_ShaderServer; }

    SharedLibrary& GetLibrary() { return m_Library; }
private:
    SharedLibrary m_Library;
    ShaderServer m_ShaderServer;
};
