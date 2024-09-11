#pragma once
#include "D3DUtil.h"
#include "Core/Defines.h"

class BufferCommon {
protected:
	ID3D12Resource* m_Resource{ nullptr };
	ID3D12Resource* m_Staging{ nullptr };
	uint32 m_ByteSize{ 0 };
public:
	BufferCommon(D3D12_HEAP_TYPE heapType, D3D12_RESOURCE_STATES states, uint32 byteSize, const void* data);
	virtual ~BufferCommon();
	ID3D12Resource* Resource() { return m_Resource; }
	uint32 ByteSize() { return m_ByteSize; }
};

class VertexBuffer: public BufferCommon {
private:
	uint32 m_Stride{ 0 };

public:
	VertexBuffer(uint32 stride, uint32 size, const void* data);
	D3D12_VERTEX_BUFFER_VIEW BufferView();
};

class IndexBuffer: public BufferCommon {
private:
	DXGI_FORMAT m_IndexFormat;
public:
	IndexBuffer(uint32 byteSize, DXGI_FORMAT indexFormat, const void* data);
	D3D12_INDEX_BUFFER_VIEW BufferView();
};


class ConstantBuffer: public BufferCommon {
private:
	BYTE* m_Mapped{nullptr};
public:
	ConstantBuffer(uint32 byteSize, const void* data);
	void UpdateData(uint32 byteSize, const void* data);
};