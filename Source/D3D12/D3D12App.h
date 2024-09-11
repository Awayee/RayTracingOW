#pragma once
#include <vector>
#include <functional>
#include <memory>
#include "D3DUtil.h"
#include "Core/Defines.h"
#include "Math/Vector.h"

class D3D12Window {
	SINGLETON_INSTANCE(D3D12Window);
public:
	Math::USize GetWindowSize() const { return m_WindowSize; }
	HWND GetWindow() { return m_MainWnd; }
	bool InitMainWindow();
	bool Tick();
private:
	HINSTANCE m_HAppInst{ nullptr };
	HWND m_MainWnd{ 0 };
	bool m_AppPaused{ false };
	bool m_Minimized{ false };
	bool m_Maximized{ false };
	Math::USize m_WindowSize{ 1280, 720 };
	bool m_Resizing{ false };
	D3D12Window(HINSTANCE hInstance, Math::USize size);
	~D3D12Window();
	LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam); //window loop func
	friend LRESULT WindowMainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	void OnMouseDown(WPARAM btnState, int x, int y);
	void OnMouseUp(WPARAM btnState, int x, int y);
	void OnMouseMove(WPARAM btnState, int x, int y);
	void OnResize();
};

class D3D12App {
public:
	static D3D12App* Instance();
	static void Initialize();
	static void Release();

	typedef std::function<void(ID3D12GraphicsCommandList*)> GFXCmdFunc;
	void ExecuteDrawCall(GFXCmdFunc&& func);
	void BeginRender();
	void EndRender();//call after BeginRender
	void ImmediatelyCommit(GFXCmdFunc&& func);
	DXGI_FORMAT GetSwapchainFormat() { return m_BackBufferFormat; }
	DXGI_FORMAT GetDepthFormat() { return m_DepthStencilFormat; }
	uint32 GetMulitSampleCount() { return m_EnableMsaa ? m_MsaaSampleCount : 1; }
	uint32 GetMulitiSampleQuality() { return m_MsaaQuality; }
	ID3D12Device* Device() { return m_Device; }
	ID3D12GraphicsCommandList* GraphicsCommandList() { return m_GraphicsCommandList; }

private:
	friend std::default_delete<D3D12App>;
	static std::unique_ptr<D3D12App> s_Instance;
	static const uint8 BACK_BUFFER_COUNT{ 2 };

	bool      m_EnableMsaa{ false };
	uint32    m_MsaaSampleCount{ 4 };
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

	D3D12App();
	~D3D12App();
	D3D12App(const D3D12App&) = delete;
	D3D12App(D3D12App&&) = delete;
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

#define DX_DEVICE (D3D12App::Instance()->Device())
