#include "ShaderCompiler.hpp"
#include "ShaderManager.hpp"

#include <Termina/Renderer/Renderer.hpp>
#include <Termina/Core/Application.hpp>
#include <Termina/Core/Logger.hpp>

#if !defined(TRMN_WINDOWS)
    #include <DXC/WinAdapter.h>
#else
    #include <Windows.h>
#endif
#include <DXC/dxcapi.h>

namespace Termina {
    typedef HRESULT (*PFN_DxcCreateInstance)(REFCLSID rclsid, REFIID riid, LPVOID *ppv);

    template<typename T>
    class DxcPtr {
        T* m_ptr = nullptr;
    public:
        DxcPtr() = default;
        DxcPtr(const DxcPtr&) = delete;
        DxcPtr& operator=(const DxcPtr&) = delete;
        ~DxcPtr() { if (m_ptr) m_ptr->Release(); }
        T* operator->() const { return m_ptr; }
        explicit operator bool() const { return m_ptr != nullptr; }
        T* Get() const { return m_ptr; }
        T** GetAddressOf() { return &m_ptr; }
    };

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
        ShaderManager* manager = Application::GetSystem<ShaderManager>();
        PFN_DxcCreateInstance dxcCreate = (PFN_DxcCreateInstance)manager->GetLibrary().GetSymbol("DxcCreateInstance");

        wchar_t wideTarget[512] = {0};
        swprintf_s(wideTarget, 512, L"%hs", ProfileFromType(args.Type));

        wchar_t wideEntry[512] = {0};
        swprintf_s(wideEntry, 512, L"%hs", args.EntryPoint.c_str());

        wchar_t widePath[512] = {0};
        swprintf_s(widePath, 512, L"%hs", args.Path.c_str());

        DxcPtr<IDxcUtils> pUtils;
        DxcPtr<IDxcCompiler3> pCompiler;
        DxcPtr<IDxcIncludeHandler> pIncludeHandler;
        DxcPtr<IDxcResult> pResult;
        DxcPtr<IDxcBlob> pShaderBlob;
        DxcPtr<IDxcBlobUtf8> pErrorsU8;

        ShaderCompiler::Result result = {};
        result.Success = false;

        // Create DXC utils and compiler
        if (FAILED(dxcCreate(CLSID_DxcUtils, IID_PPV_ARGS(pUtils.GetAddressOf())))) {
            TN_ERROR("Failed to create DXC utils!");
            return result;
        }
        if (FAILED(dxcCreate(CLSID_DxcCompiler, IID_PPV_ARGS(pCompiler.GetAddressOf())))) {
            TN_ERROR("Failed to create DXC compiler!");
            return result;
        }

        // Create default include handler
        if (FAILED(pUtils->CreateDefaultIncludeHandler(pIncludeHandler.GetAddressOf()))) {
            TN_ERROR("Failed to create include handler!");
            return result;
        }

        // Setup source buffer
        DxcBuffer sourceBuffer = {};
        sourceBuffer.Ptr = args.Source.c_str();
        sourceBuffer.Size = args.Source.size();
        sourceBuffer.Encoding = DXC_CP_UTF8;

        bool hasRaytracing = Application::GetSystem<RendererSystem>()->GetDevice()->SupportsRaytracing();

        // Prepare arguments
        std::vector<LPCWSTR> compileArgs = {};
        std::vector<std::wstring> wideDefines; // Keep strings alive until after compile
        compileArgs.push_back(widePath);
        compileArgs.push_back(L"-E");
        compileArgs.push_back(wideEntry);
        compileArgs.push_back(L"-T");
        compileArgs.push_back(wideTarget);
    #if !defined(TRMN_RETAIL)
        compileArgs.push_back(L"-Qembed_debug");
        compileArgs.push_back(L"-Zi");
    #endif
        if (hasRaytracing) {
            compileArgs.push_back(L"-DRAYTRACING");
        }

#if !defined(TRMN_MACOS)
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
#else
        compileArgs.push_back(L"-DMETAL");
#endif

        for (const std::string& define : args.Defines) {
            wideDefines.push_back(L"-D" + std::wstring(define.begin(), define.end()));
            compileArgs.push_back(wideDefines.back().c_str());
        }

        HRESULT hresult = pCompiler->Compile(&sourceBuffer, compileArgs.data(), (uint32)compileArgs.size(), pIncludeHandler.Get(), IID_PPV_ARGS(pResult.GetAddressOf()));
        if (FAILED(hresult)) {
            TN_ERROR("Failed to start shader compilation: %08X", hresult);
            return result;
        }

        // Get errors
        if (SUCCEEDED(pResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(pErrorsU8.GetAddressOf()), nullptr)) && pErrorsU8 && pErrorsU8->GetStringLength() > 0) {
            TN_ERROR("Shader errors: %s", pErrorsU8->GetStringPointer());
        }

        // Check compilation status
        HRESULT status = S_OK;
        if (FAILED(pResult->GetStatus(&status)) || FAILED(status)) {
            TN_ERROR("Shader compilation failed (HRESULT=0x%08X)", status);
            return result;
        }

        // Get result blob
        if (FAILED(pResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(pShaderBlob.GetAddressOf()), nullptr)) || !pShaderBlob) {
            TN_ERROR("Shader compilation succeeded but no shader blob was produced!");
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

        return result;
    }
}
