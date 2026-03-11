#include "ShaderCompiler.h"
#include "Engine/Renderer/RendererSubsystem.h"
#include "ShaderManager.h"

#include "Engine/Core/Application.h"

#if defined(PLATFORM_LINUX)
    #include "DXC/include/dxc/WinAdapter.h"
#elif defined(PLATFORM_WINDOWS)
    #include <Windows.h>
#endif
#include "DXC/include/dxc/dxcapi.h"

typedef HRESULT (*PFN_DxcCreateInstance)(REFCLSID rclsid, REFIID riid, LPVOID *ppv);

inline const char* ProfileFromType(ShaderType type)
{
    switch (type)
    {
    case ShaderType::VERTEX:
        return "vs_6_6";
    case ShaderType::PIXEL:
        return "ps_6_6";
    case ShaderType::COMPUTE:
        return "cs_6_6";
    case ShaderType::MESH:
        return "ms_6_6";
    case ShaderType::TASK:
        return "as_6_6";
    default:
        return "";
    }
}

ShaderCompiler::Result ShaderCompiler::Compile(const ShaderCompiler::Arguments& args)
{
    ShaderManager* manager = Application::Get()->GetSubsystem<ShaderManager>();
    PFN_DxcCreateInstance dxcCreate = (PFN_DxcCreateInstance)manager->GetLibrary().GetSymbol("DxcCreateInstance");

    const char* sourceCstr = args.Source.c_str();

    wchar_t wideTarget[512] = {0};
    swprintf_s(wideTarget, 512, L"%hs", ProfileFromType(args.Type));

    wchar_t wideEntry[512] = {0};
    swprintf_s(wideEntry, 512, L"%hs", args.EntryPoint.c_str());

    wchar_t widePath[512] = {0};
    swprintf_s(widePath, 512, L"%hs", args.Path.c_str());

    IDxcUtils* pUtils = nullptr;
    IDxcCompiler* pCompiler = nullptr;
    IDxcBlobEncoding* pSourceBlob = nullptr;
    IDxcIncludeHandler* pIncludeHandler = nullptr;
    IDxcOperationResult* pResult = nullptr;
    IDxcBlob* pShaderBlob = nullptr;
    IDxcBlobEncoding* pErrors = nullptr;
    IDxcBlobUtf8* pErrorsU8 = nullptr;

    ShaderCompiler::Result result = {};
    result.Success = false;

    // Create DXC utils and compiler
    if (FAILED(dxcCreate(CLSID_DxcUtils, IID_PPV_ARGS(&pUtils)))) {
        LOG_ERROR("Failed to create DXC utils!");
        return result;
    }
    if (FAILED(dxcCreate(CLSID_DxcCompiler, IID_PPV_ARGS(&pCompiler)))) {
        LOG_ERROR("Failed to create DXC compiler!");
        return result;
    }

    // Create default include handler
    if (FAILED(pUtils->CreateDefaultIncludeHandler(&pIncludeHandler))) {
        LOG_ERROR("Failed to create include handler!");
        return result;
    }

    // Create source blob
    if (FAILED(pUtils->CreateBlob(sourceCstr, (uint32)args.Source.size(), 0, &pSourceBlob))) {
        LOG_ERROR("Failed to create source blob!");
        return result;
    }

    bool hasRaytracing = Application::Get()->GetSubsystem<RendererSubsystem>()->GetDevice()->SupportsRaytracing();

    // Prepare arguments
    std::vector<LPCWSTR> compileArgs = {};
    std::vector<std::wstring> wideDefines; // Keep strings alive until after compile
#if defined(_DEBUG)
    compileArgs.push_back(L"-Qembed_debug");
    compileArgs.push_back(L"-Zi");
#endif

    if (hasRaytracing) {
        compileArgs.push_back(L"-DRAYTRACING");
    }

#if defined(PLATFORM_LINUX) // Vulkan
    compileArgs.push_back(L"-DVULKAN");
    compileArgs.push_back(L"-spirv");
    compileArgs.push_back(L"-fspv-extension=SPV_EXT_mesh_shader");
    compileArgs.push_back(L"-fspv-extension=SPV_EXT_descriptor_indexing");
    compileArgs.push_back(L"-fspv-extension=SPV_KHR_ray_query");
    compileArgs.push_back(L"-fspv-extension=SPV_KHR_shader_draw_parameters");
    compileArgs.push_back(L"-fspv-target-env=vulkan1.3");
    compileArgs.push_back(L"-fvk-use-scalar-layout");
    compileArgs.push_back(L"-fvk-bind-resource-heap");
    compileArgs.push_back(L"0");
    compileArgs.push_back(L"0");
    compileArgs.push_back(L"-fvk-bind-sampler-heap");
    compileArgs.push_back(L"1");
    compileArgs.push_back(L"0");
#elif defined (PLATFORM_MACOS) // Metal
    compileArgs.push_back(L"-DMETAL");
#elif defined (PLATFORM_WINDOWS) // DirectX 12
    compileArgs.push_back(L"-DD3D12");
#endif

    for (const std::string& define : args.Defines) {
        wideDefines.push_back(L"-D" + std::wstring(define.begin(), define.end()));
        compileArgs.push_back(wideDefines.back().c_str());
    }

    if (FAILED(pCompiler->Compile(pSourceBlob, widePath, wideEntry, wideTarget, compileArgs.data(), (uint32)compileArgs.size(), nullptr, 0, pIncludeHandler, &pResult))) {
        LOG_ERROR("Failed to start shader compilation!");
        return result;
    }

    // Get error buffer
    if (SUCCEEDED(pResult->GetErrorBuffer(&pErrors)) && pErrors && pErrors->GetBufferSize() > 0) {
        if (SUCCEEDED(pErrors->QueryInterface(IID_PPV_ARGS(&pErrorsU8))) && pErrorsU8) {
            LOG_ERROR("Shader errors: %s", (char*)pErrorsU8->GetStringPointer());
        }
    }

    // Check compilation status
    HRESULT status = S_OK;
    if (FAILED(pResult->GetStatus(&status)) || FAILED(status)) {
        LOG_ERROR("Shader compilation failed (HRESULT=0x%08X)", status);
        return result;
    }

    // Get result blob
    if (FAILED(pResult->GetResult(&pShaderBlob)) || !pShaderBlob) {
        LOG_ERROR("Shader compilation succeeded but no shader blob was produced!");
        return result;
    }

    // Fill result
    result.EntryPoint = args.EntryPoint;
    result.Type = args.Type;
    result.Bytecode.resize(pShaderBlob->GetBufferSize());
    memcpy(result.Bytecode.data(), pShaderBlob->GetBufferPointer(), result.Bytecode.size());
    result.Success = true;

    if (args.PostProcessCallback) {
        // Mostly used for converting DXIL to Metal library
        result.Bytecode = args.PostProcessCallback(result.Bytecode, result.EntryPoint);
    }

    if (pShaderBlob) pShaderBlob->Release();
    if (pErrorsU8) pErrorsU8->Release();
    if (pErrors) pErrors->Release();
    if (pResult) pResult->Release();
    if (pSourceBlob) pSourceBlob->Release();
    if (pIncludeHandler) pIncludeHandler->Release();
    if (pCompiler) pCompiler->Release();
    if (pUtils) pUtils->Release();

    return result;
}
