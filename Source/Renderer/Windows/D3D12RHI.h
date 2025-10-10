#pragma once
#include <functional>
#include "Core/Defines.h"
#include "Core/TUniquePtr.h"
#include "Renderer/Windows/D3DUtil.h"
#include "Renderer/Windows/D3D12RHI.h"
#include "Renderer/RHICommon.h"

class D3D12Window;
class D3D12PSO;

class D3D12RHI {
public:
	D3D12RHI(uint32 WindowWidth, uint32 WindowHeight);
	~D3D12RHI();
	typedef std::function<void(ID3D12GraphicsCommandList*)> GFXCmdFunc;
	void ExecuteDrawCall(GFXCmdFunc&& func);
	void BeginRender();
	void EndRender();//call after BeginRender
	void ImmediatelyCommit(GFXCmdFunc&& func);

	// public func
	RHITextureHandle CreateTexture(uint32 width, uint32 height, uint16 layer, uint16 mip, ERHIFormat format);
	void UpdateTextureData(RHITextureHandle Handle, const void* Data, uint64 ByteSize);
	void DestroyTexture(RHITextureHandle Handle);
	bool DrawTexture(RHITextureHandle TextureHandle);

private:
	static const uint8 BACK_BUFFER_COUNT{ 2 };
	TUniquePtr<D3D12Window> m_Window;
	bool      m_EnableMsaa{ false };
	uint32    m_MsaaSampleCount{ 1 };
	uint32    m_MsaaQuality{ 0 };

	IDXGIFactory4* m_DXGIFactory;
	ID3D12Device* m_Device{ nullptr };
	IDXGISwapChain* m_Swapchain{ nullptr };
	ID3D12Fence* m_Fence{ nullptr };
	uint32 m_CurrentFence{ 0 };
	ID3D12CommandQueue* m_CommandQueue{ nullptr };
	ID3D12CommandAllocator* m_DirectCmdListAlloc{ nullptr };
	ID3D12CommandAllocator* m_CommandAllocator{ nullptr };
	ID3D12GraphicsCommandList* m_GraphicsCommandList{ nullptr };
	ID3D12Resource* m_SwapchainBuffer[BACK_BUFFER_COUNT]{nullptr, nullptr};
	int m_CurrentBuffer{ 1 };
	uint32 m_RtvDescriptorSize = 0;//Render Target View
	//uint32 m_DsvDescriptorSize = 0;//Depth Stencil view
	//uint32 m_CbvSrvUavDescriptorSize = 0;
	ID3D12DescriptorHeap* m_RTVDescriptorHeap;
	D3D12_VIEWPORT m_Viewport;
	D3D12_RECT m_ScissorRect;
	D3D_DRIVER_TYPE m_DriverType{ D3D_DRIVER_TYPE_HARDWARE };
	DXGI_FORMAT m_BackBufferFormat{ DXGI_FORMAT_R8G8B8A8_UNORM };
	DXGI_FORMAT m_DepthStencilFormat{ DXGI_FORMAT_D24_UNORM_S8_UINT };
	TUniquePtr<D3D12PSO> DefaultPSO;
	
	D3D12RHI(const D3D12RHI&) = delete;
	D3D12RHI(D3D12RHI&&) = delete;
	void InitializeDX();
	void CreateCommandObjects();
	void CreateSwapchain();
	void CreateInternalDescriptorHeaps();
	void FlushCommandQueue();
	void CreateSwapchainBuffers();
	ID3D12Resource* BackBuffer() { return m_SwapchainBuffer[m_CurrentBuffer]; }
	D3D12_CPU_DESCRIPTOR_HANDLE BackBufferView();
	void OnResize();
};
