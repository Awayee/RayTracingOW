#include "Buffer.h"
#include "D3D12App.h"
#include "Core/Defines.h"

BufferCommon::BufferCommon(D3D12_HEAP_TYPE heapType, D3D12_RESOURCE_STATES states, uint32 byteSize, const void* data):m_ByteSize(byteSize) {
	DX_CHECK(DX_DEVICE->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(heapType),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(m_ByteSize),
		states,
		nullptr,
		IID_PPV_ARGS(&m_Resource)));

	if(!data) {
		return;
	}
	// stating buffer is used to copy CPU memory data into our default buffer
	DX_CHECK(DX_DEVICE->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(m_ByteSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_Staging)));

	// transition buffers
	ID3D12GraphicsCommandList* cmdList = D3D12App::Instance()->GraphicsCommandList();
	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_Resource, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST));
	// Describe the data we want to copy into the default buffer.
	D3D12_SUBRESOURCE_DATA subResourceData = {};
	subResourceData.pData = data;
	subResourceData.RowPitch = m_ByteSize;
	subResourceData.SlicePitch = subResourceData.RowPitch;
	UpdateSubresources<1>(cmdList, m_Resource, m_Staging, 0, 0, 1, &subResourceData);
	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_Resource, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ));
}

BufferCommon::~BufferCommon() {
	DX_RELEASE(m_Resource);
	DX_RELEASE(m_Staging);
}

VertexBuffer::VertexBuffer(uint32 stride, uint32 size, const void* data):
BufferCommon(D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_COMMON, stride*size, data), m_Stride(stride) {}

D3D12_VERTEX_BUFFER_VIEW VertexBuffer::BufferView() {
	D3D12_VERTEX_BUFFER_VIEW v;
	v.BufferLocation = m_Resource->GetGPUVirtualAddress();
	v.StrideInBytes = m_Stride;
	v.SizeInBytes = m_ByteSize;
	return v;
}

IndexBuffer::IndexBuffer(uint32 byteSize, DXGI_FORMAT indexFormat, const void* data):
BufferCommon(D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_COMMON, byteSize, data), m_IndexFormat(indexFormat) {}

D3D12_INDEX_BUFFER_VIEW IndexBuffer::BufferView() {
	D3D12_INDEX_BUFFER_VIEW v;
	v.BufferLocation = m_Resource->GetGPUVirtualAddress();
	v.Format = m_IndexFormat;
	v.SizeInBytes = m_ByteSize;
	return v;
}

ConstantBuffer::ConstantBuffer(uint32 byteSize, const void* data):
BufferCommon(D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ, CalcConstantBufferByteSize(byteSize), data) {
	//DX_CHECK(m_Resource->Map(0, nullptr, reinterpret_cast<void**>(&m_Mapped)));
}

void ConstantBuffer::UpdateData(uint32 byteSize, const void* data) {
	void* mappedData;
	DX_CHECK(m_Resource->Map(0, nullptr, &mappedData));
	memcpy(mappedData, data, byteSize);
	m_Resource->Unmap(0, nullptr);
}
