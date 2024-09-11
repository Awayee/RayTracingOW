#pragma once
#include "D3DUtil.h"
#include "Core/Defines.h"

class Texture2D {
private:
	ID3D12Resource* m_Resource{ nullptr };
	ID3D12DescriptorHeap* m_SrvDescriptorHeap{ nullptr };
public:
	Texture2D(uint32 width, uint32 height, uint16 layer, uint16 mip, DXGI_FORMAT format);
	~Texture2D();
	void UpdateData(ID3D12GraphicsCommandList* cmdList, const void* byteData, uint32 byteSize);
	ID3D12Resource* Resource() { return m_Resource; }
	void BindDesc(ID3D12GraphicsCommandList* cmdList, uint32 slot);
};
