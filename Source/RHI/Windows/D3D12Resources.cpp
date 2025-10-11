#include "RHI/Windows/D3D12Resources.h"
#include "RHI/ShaderCompiler.h"
#include "Core/Defines.h"
#include "Core/Log.h"
#include <array>
#include <vector>

inline std::string GetCompiledShaderFileName(const char* FileName, const char* EntryName) {
	std::string Output{ FileName };
	if (size_t ExtIdx = Output.rfind(".hlsl"); ExtIdx != std::string::npos) {
		Output.erase(ExtIdx);
	}
	Output.append(EntryName).append(".cso");
	return Output;
}

D3D12Buffer::D3D12Buffer(ID3D12Device* device, D3D12_HEAP_TYPE heapType, D3D12_RESOURCE_STATES states, size_t byteSize):m_ByteSize(byteSize) {
	DX_CHECK(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(heapType),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(m_ByteSize),
		states,
		nullptr,
		IID_PPV_ARGS(&m_Resource)));
	// stating buffer is used to copy CPU memory data into our default buffer
	DX_CHECK(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(m_ByteSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_Staging)));
}

void D3D12Buffer::UpdateStaticData(ID3D12GraphicsCommandList* cmdList, const void* data) {
	if (!data) {
		return;
	}
	// transition buffers
	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_Resource, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST));
	// Describe the data we want to copy into the default buffer.
	D3D12_SUBRESOURCE_DATA subResourceData = {};
	subResourceData.pData = data;
	subResourceData.RowPitch = m_ByteSize;
	subResourceData.SlicePitch = subResourceData.RowPitch;
	UpdateSubresources<1>(cmdList, m_Resource, m_Staging, 0, 0, 1, &subResourceData);
	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_Resource, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ));
}

D3D12Buffer::~D3D12Buffer() {
	DX_RELEASE(m_Resource);
	DX_RELEASE(m_Staging);
}


D3D12Texture::D3D12Texture(ID3D12Device* device, uint32 width, uint32 height, uint16 layer, uint16 mip, DXGI_FORMAT format) {
	D3D12_RESOURCE_DESC desc{};
	desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	desc.Width = width;
	desc.Height = height;
	desc.DepthOrArraySize = layer;
	desc.MipLevels = mip;
	desc.Format = format;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Flags = D3D12_RESOURCE_FLAG_NONE;
	CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_DEFAULT);
	device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&m_Resource));
	// descriptor heap
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.NumDescriptors = 1;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	DX_CHECK(device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_SrvDescriptorHeap)));
	// srv
	D3D12_SHADER_RESOURCE_VIEW_DESC srv{};
	srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srv.Format = format;
	srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srv.Texture2D.MipLevels = 1;
	device->CreateShaderResourceView(m_Resource, &srv, m_SrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
}

D3D12Texture::~D3D12Texture() {
	DX_RELEASE(m_Resource);
}

void D3D12Texture::UpdateData(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, const void* byteData, size_t byteSize) {
	// get required buffer size
	const auto requiredSize = GetRequiredIntermediateSize(m_Resource, 0, 1);
	// create upload buffer
	ID3D12Resource* uploadBuffer;
	CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_UPLOAD);
	CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(requiredSize);
	device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&uploadBuffer));
	// copy data to upload buffer
	D3D12_SUBRESOURCE_DATA data{};
	data.pData = byteData;
	data.RowPitch = m_Resource->GetDesc().Width * 4;
	data.SlicePitch = byteSize;
	UINT64 result = UpdateSubresources(cmdList, m_Resource, uploadBuffer, 0, 0, 1, &data);
	if (FAILED(result)) {
		LOG_ERROR("UpdateSubresources failed!");
	}
	// transition state
	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_Resource, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	cmdList->ResourceBarrier(1, &barrier);
}

void D3D12Texture::BindDesc(ID3D12GraphicsCommandList* cmdList, uint32 slot) {
	cmdList->SetDescriptorHeaps(1, &m_SrvDescriptorHeap);
	CD3DX12_GPU_DESCRIPTOR_HANDLE tex(m_SrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
	cmdList->SetGraphicsRootDescriptorTable(slot, tex);
}

HRESULT LoadShaderFile(const char* ShaderFileName, const char* EntryName, const char* ShaderModule, std::vector<char>& OutBytes) {
	std::string CompiledFileName = GetCompiledShaderFileName(ShaderFileName, EntryName);
	std::string CompiledFullPath = std::string{SHADER_PATH}.append(CompiledFileName);

	if(!ShaderCompiler::ReadCompiledShaderFile(CompiledFullPath.c_str(), EntryName, ShaderModule, OutBytes)) {
		std::string ShaderFullPath = std::string{SHADER_PATH}.append(ShaderFileName);
		const std::vector<ShaderCompiler::Macro> Defines{ {"RHI_D3D12", "1"} };
		ShaderCompiler::CompileShaderSigned(ShaderFullPath.c_str(), EntryName, ShaderModule, Defines, OutBytes);
		ShaderCompiler::SaveCompiledShaderFile(CompiledFullPath.c_str(), OutBytes);
	}
	return OutBytes.empty() ? -1 : S_OK;
	/*
	// .hlsl to .cso
	std::wstring hlslFile{ SHADER_PATH_W };
	hlslFile.append(ShaderFileName);
	std::wstring csoFile = hlslFile;
	// char to wchar_t
	std::wstring entryPointW;
	const size_t entryPointLen = strlen(EntryName);
	entryPointW.resize(entryPointLen);
	MultiByteToWideChar(CP_ACP, 0, EntryName, entryPointLen, entryPointW.data(), 32);

	csoFile.append(entryPointW);
	csoFile.append(L".cso");
#ifndef _DEBUG
	if (D3DReadFileToBlob(csoFile.c_str(), ppBlobOut) == S_OK) {
		return S_OK;
	}
#endif
	hlslFile.append(L".hlsl");
	DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
	// save debug info
	dwShaderFlags |= D3DCOMPILE_DEBUG;
	// disable optimization when debug
	dwShaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
	ID3DBlob* errorBlob = nullptr;
	HRESULT hr = D3DCompileFromFile(hlslFile.c_str(), nullptr, nullptr, (LPCSTR)EntryName, ShaderModule, dwShaderFlags, 0, ppBlobOut, &errorBlob);
	if (FAILED(hr)) {
		if (errorBlob != nullptr) {
			LOG(reinterpret_cast<const char*>(errorBlob->GetBufferPointer()));
		}
		LOG("Failed to load shader: %ls", hlslFile.c_str());
		DX_RELEASE(errorBlob);
		return hr;
	}
	return D3DWriteBlobToFile(*ppBlobOut, csoFile.c_str(), TRUE);
	*/
}

D3D12PSO::D3D12PSO(ID3D12Device* device, const char* fileName, DXGI_FORMAT RTFormat, DXGI_FORMAT DepthFormat, UINT MultiSampleCount, UINT MultiSampleQuality) {
	// Create root signature
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
	DX_CHECK(device->CreateRootSignature(
		0,
		serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(&m_RootSignature)));

	// Create pso
	std::vector<char> VSBytes;
	DX_CHECK(LoadShaderFile(fileName, "MainVS", "vs_6_0", VSBytes));
	std::vector<char> PSBytes;
	DX_CHECK(LoadShaderFile(fileName, "MainPS", "ps_6_0", PSBytes));

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
	psoDesc.InputLayout = { nullptr, 0 };
	psoDesc.pRootSignature = m_RootSignature;
	psoDesc.VS.pShaderBytecode = reinterpret_cast<BYTE*>(VSBytes.data());
	psoDesc.VS.BytecodeLength = VSBytes.size();
	psoDesc.PS.pShaderBytecode = reinterpret_cast<BYTE*>(PSBytes.data());
	psoDesc.PS.BytecodeLength = PSBytes.size();

	//cull back face
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.RasterizerState.FrontCounterClockwise = true;
	psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;

	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = RTFormat;
	psoDesc.SampleDesc.Count = MultiSampleCount;
	psoDesc.SampleDesc.Quality = MultiSampleQuality - 1;
	psoDesc.DSVFormat = DepthFormat;
	DX_CHECK(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_PSO)));

}

D3D12PSO::~D3D12PSO() {
	DX_RELEASE(m_PSO);
	DX_RELEASE(m_RootSignature);
}

void D3D12PSO::Bind(ID3D12GraphicsCommandList* cmdList) {
	cmdList->SetPipelineState(m_PSO);
	cmdList->SetGraphicsRootSignature(m_RootSignature);
}
