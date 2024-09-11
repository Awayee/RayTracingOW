#include "Texture.h"
#include "D3D12/D3D12App.h"
#include "Core/Log.h"

Texture2D::Texture2D(uint32 width, uint32 height, uint16 layer, uint16 mip, DXGI_FORMAT format) {
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
	ID3D12Device* device = D3D12App::Instance()->Device();
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

Texture2D::~Texture2D() {
	DX_RELEASE(m_Resource);
}

void Texture2D::UpdateData(ID3D12GraphicsCommandList* cmdList, const void* byteData, uint32 byteSize) {
	ID3D12Device* device = D3D12App::Instance()->Device();
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
	if(FAILED(result)) {
		LOG_ERROR("UpdateSubresources failed!");
	}
	// transition state
	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_Resource, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	cmdList->ResourceBarrier(1, &barrier);
}

void Texture2D::BindDesc(ID3D12GraphicsCommandList* cmdList, uint32 slot) {
	cmdList->SetDescriptorHeaps(1, &m_SrvDescriptorHeap);
	CD3DX12_GPU_DESCRIPTOR_HANDLE tex(m_SrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
	cmdList->SetGraphicsRootDescriptorTable(slot, tex);
}
