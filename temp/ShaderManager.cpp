#include "ShaderManager.h"

#include "Engine/Core/Application.h"
#include "Engine/Renderer/RendererSubsystem.h"
#include "Engine/Core/DebugUI/DebugUIManager.h"

#if defined(PLATFORM_MACOS)
    #include "Metal/Metal_ShaderConverter.h"
#endif

bool ShaderManager::Initialize()
{
    std::string libraryPath = "dxcompiler.dll";
#if defined(PLATFORM_LINUX)
    libraryPath = "libdxcompiler.so";
#elif defined(PLATFORM_MACOS)
    libraryPath = "libdxcompiler.dylib";
#endif

    m_Library.Open(libraryPath.c_str());
    m_ShaderServer.ConnectDevice(Application::Get()->GetSubsystem<RendererSubsystem>()->GetDevice());
    
#if defined(PLATFORM_MACOS)
    MetalShaderConverter::Initialize();
#endif

    DebugUIManager::Get().RegisterPanel(
		{"Renderer", "Shader Manager"},
		[&]() {
			m_ShaderServer.RenderDebugUI();
		},
		false
	);

    return true;
}

void ShaderManager::Shutdown()
{
    m_ShaderServer.Clear();
#if defined(PLATFORM_MACOS)
    MetalShaderConverter::Shutdown();
#endif

}

void ShaderManager::PostRender()
{
#if defined(_DEBUG)
    m_ShaderServer.ReloadModifiedPipelines();
#endif
}
