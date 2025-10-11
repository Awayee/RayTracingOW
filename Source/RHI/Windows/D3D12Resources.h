#pragma once
#include "RHI/Windows/D3DUtil.h"
#include "Core/Defines.h"

class D3D12Buffer {
protected:
	ID3D12Resource* m_Resource{ nullptr };
	ID3D12Resource* m_Staging{ nullptr };
	uint32 m_ByteSize{ 0 };
public:
	D3D12Buffer(ID3D12Device* device, D3D12_HEAP_TYPE heapType, D3D12_RESOURCE_STATES states, size_t byteSize);
	void UpdateStaticData(ID3D12GraphicsCommandList* cmdList, const void* data);
	virtual ~D3D12Buffer();
	ID3D12Resource* Resource() { return m_Resource; }
	uint32 ByteSize() { return m_ByteSize; }
};

class D3D12Texture {
private:
	ID3D12Resource* m_Resource{ nullptr };
	ID3D12DescriptorHeap* m_SrvDescriptorHeap{ nullptr };
public:
	D3D12Texture(ID3D12Device* device, uint32 width, uint32 height, uint16 layer, uint16 mip, DXGI_FORMAT format);
	~D3D12Texture();
	void UpdateData(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, const void* byteData, size_t byteSize);
	ID3D12Resource* Resource() { return m_Resource; }
	void BindDesc(ID3D12GraphicsCommandList* cmdList, uint32 slot);
};

class D3D12PSO {
private:
	ID3D12RootSignature* m_RootSignature{ nullptr };
	ID3D12PipelineState* m_PSO{ nullptr };
public:
	// args: shader path, with out file extent.
	D3D12PSO(ID3D12Device* device, const char* fileName, DXGI_FORMAT RTFormat, DXGI_FORMAT DepthFormat, UINT MultiSampleCount, UINT MultiSampleQuality);
	~D3D12PSO();
	virtual void Bind(ID3D12GraphicsCommandList* cmdList);
};