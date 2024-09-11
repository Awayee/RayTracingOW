#include "Pipeline.h"
#include "D3D12App.h"
#include <d3dcompiler.h>
#include <string>
#include <array>

#define SHADER_MODEL_VS "vs_5_1"
#define SHADER_MODEL_PS "ps_5_1"

#define PARSE_SHADER_FILE(f)\
	char __s[128]; strcpy(__s, SHADER_PATH); strcat(__s, f); f=__s

#define SHADER_FILE_EXT L".hlsl"
#define SHADER_COMPILED_FILE_EXT L".cso"

HRESULT LoadShaderFile(const WCHAR* file, const LPCTSTR entryPoint, LPCTSTR shaderModel, ID3DBlob** ppBlobOut) {
    // .hlsl to .cso
    std::wstring hlslFile{ SHADER_PATH_W };
    hlslFile.append(file);
    std::wstring csoFile = hlslFile;
    // char to wchar_t
    std::wstring entryPointW;
    const size_t entryPointLen = strlen(entryPoint);
    entryPointW.resize(entryPointLen);
    MultiByteToWideChar(CP_ACP, 0, entryPoint, entryPointLen, entryPointW.data(), 32);

    csoFile.append(entryPointW);
    csoFile.append(SHADER_COMPILED_FILE_EXT);
#ifndef _DEBUG
    if (D3DReadFileToBlob(csoFile.c_str(), ppBlobOut) == S_OK) {
        return S_OK;
    }
#endif
    hlslFile.append(SHADER_FILE_EXT);
    DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
    // save debug info
    dwShaderFlags |= D3DCOMPILE_DEBUG;
    // disable optimization when debug
    dwShaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
    ID3DBlob* errorBlob = nullptr;
    HRESULT hr = D3DCompileFromFile(hlslFile.c_str(), nullptr, nullptr, (LPCSTR)entryPoint, shaderModel, dwShaderFlags, 0, ppBlobOut, &errorBlob);
    if (FAILED(hr)) {
        if (errorBlob != nullptr) {
            LOG(reinterpret_cast<const char*>(errorBlob->GetBufferPointer()));
        }
        LOG("Failed to load shader: %ls", hlslFile.c_str());
        DX_RELEASE(errorBlob);
        return hr;
    }
    return D3DWriteBlobToFile(*ppBlobOut, csoFile.c_str(), TRUE);
}

void PSOCommon::CreateRootSignature() {
    std::array<CD3DX12_ROOT_PARAMETER, 1> slotRootParameters;
    CD3DX12_DESCRIPTOR_RANGE texTable{ D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0 };
    slotRootParameters[0].InitAsDescriptorTable(1, &texTable, D3D12_SHADER_VISIBILITY_PIXEL);

    //static sampler
    const CD3DX12_STATIC_SAMPLER_DESC linearClamp{
        0, // shaderRegister
        D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP }; // addressW

    CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
        slotRootParameters.size(),
        slotRootParameters.data(),
        1,
        &linearClamp,
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    // create a root signature with a single slot which points to a descriptor range consisting of a single constant buffer
    ID3DBlob* serializedRootSig = nullptr;
    ID3DBlob* errorBlob = nullptr;
    HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &serializedRootSig, &errorBlob);
    if (errorBlob != nullptr) {
        LOG((char*)errorBlob->GetBufferPointer());
    }
    DX_CHECK(hr);
    DX_CHECK(DX_DEVICE->CreateRootSignature(
        0,
        serializedRootSig->GetBufferPointer(),
        serializedRootSig->GetBufferSize(),
        IID_PPV_ARGS(&m_RootSignature)));
}

PSOCommon::PSOCommon() {
    const wchar_t* fileName = L"TextureMap";
    CreateRootSignature();
    ID3DBlob* vsb;
    DX_CHECK(LoadShaderFile(fileName, "MainVS", SHADER_MODEL_VS, &vsb));
    ID3DBlob* psb;
    DX_CHECK(LoadShaderFile(fileName, "MainPS", SHADER_MODEL_PS, &psb));

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
    psoDesc.InputLayout = { nullptr, 0 };
    psoDesc.pRootSignature = m_RootSignature;
    psoDesc.VS.pShaderBytecode = reinterpret_cast<BYTE*>(vsb->GetBufferPointer());
    psoDesc.VS.BytecodeLength = vsb->GetBufferSize();
    psoDesc.PS.pShaderBytecode = reinterpret_cast<BYTE*>(psb->GetBufferPointer());
    psoDesc.PS.BytecodeLength = psb->GetBufferSize();

    //cull back face
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.RasterizerState.FrontCounterClockwise = true;
    psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;

    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = D3D12App::Instance()->GetSwapchainFormat();
    psoDesc.SampleDesc.Count =D3D12App::Instance()->GetMulitSampleCount();
    psoDesc.SampleDesc.Quality = D3D12App::Instance()->GetMulitiSampleQuality() - 1;
    psoDesc.DSVFormat = D3D12App::Instance()->GetDepthFormat();
    DX_CHECK(DX_DEVICE->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_PSO)));

    DX_RELEASE(vsb);
    DX_RELEASE(psb);

}

PSOCommon::~PSOCommon() {
    DX_RELEASE(m_PSO);
    DX_RELEASE(m_RootSignature);
}

void PSOCommon::Bind(ID3D12GraphicsCommandList* cmdList) {
    cmdList->SetPipelineState(m_PSO);
    cmdList->SetGraphicsRootSignature(m_RootSignature);
}
