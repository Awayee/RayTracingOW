#include "RHI/Windows/D3D12RHI.h"
#include "RHI/Windows/D3D12Resources.h"
#include "RHI/Windows/D3D12Window.h"
#include "Core/Log.h"
#include <WindowsX.h>
#include <string>
#include <array>

#define DX_RELEASE(x) if(x)(x)->Release(); (x)=nullptr

#if defined(DEBUG) | defined(_DEBUG)
#define DX_CHECK(x)\
    do{\
        HRESULT __hr = (x);\
        if(FAILED(__hr)){\
            DXTraceWDetail(__FILEW__, (DWORD)__LINE__, __hr, L#x);\
            ASSERT(0, "");\
        }\
    }while(0)
#else
#define DX_CHECK(x) (x)
#endif


#ifdef _DEBUG
void LogOutputDisplayModes(IDXGIOutput* output, DXGI_FORMAT format)
{
	UINT count = 0;
	UINT flags = 0;

	// Call with nullptr to get list count.
	output->GetDisplayModeList(format, flags, &count, nullptr);

	std::vector<DXGI_MODE_DESC> modeList(count);
	output->GetDisplayModeList(format, flags, &count, &modeList[0]);

	for (auto& x : modeList)
	{
		UINT n = x.RefreshRate.Numerator;
		UINT d = x.RefreshRate.Denominator;
		std::wstring text =
			L"Width = " + std::to_wstring(x.Width) + L" " +
			L"Height = " + std::to_wstring(x.Height) + L" " +
			L"Refresh = " + std::to_wstring(n) + L"/" + std::to_wstring(d) +
			L"\n";

		wprintf(text.c_str());
	}
}
void LogAdapterOutputs(IDXGIAdapter* adapter, DXGI_FORMAT format)
{
	UINT i = 0;
	IDXGIOutput* output = nullptr;
	while (adapter->EnumOutputs(i, &output) != DXGI_ERROR_NOT_FOUND)
	{
		DXGI_OUTPUT_DESC desc;
		output->GetDesc(&desc);

		std::wstring text = L"***Output: ";
		text += desc.DeviceName;
		text += L"\n";
		wprintf(text.c_str());

		LogOutputDisplayModes(output, format);

		DX_RELEASE(output);

		++i;
	}
}

void LogAdapters(IDXGIFactory4* factory, DXGI_FORMAT format) {
	UINT i = 0;
	IDXGIAdapter* adapter = nullptr;
	std::vector<IDXGIAdapter*> adapterList;
	while (factory->EnumAdapters(i, &adapter) != DXGI_ERROR_NOT_FOUND)
	{
		DXGI_ADAPTER_DESC desc;
		adapter->GetDesc(&desc);

		std::wstring text = L"***Adapter: ";
		text += desc.Description;
		text += L"\n";
		wprintf(text.c_str());
		adapterList.push_back(adapter);
		++i;
	}

	//for (size_t i = 0; i < adapterList.size(); ++i)
	//{
	//	LogAdapterOutputs(adapterList[i], format);
	//	DX_RELEASE(adapterList[i]);
	//}
}
#endif

inline DXGI_FORMAT Convert2DXGIFormat(ERHIFormat InFormat) {
	switch (InFormat) {
	case ERHIFormat::R8G8B8A8_UNorm: return DXGI_FORMAT_R8G8B8A8_UNORM;
	default: return DXGI_FORMAT_UNKNOWN;
	}
}

D3D12RHI::D3D12RHI(uint32 WindowWidth, uint32 WindowHeight) {
	HINSTANCE AppInstance = GetModuleHandle(0);
	m_Window.Reset(new D3D12Window(AppInstance, WindowWidth, WindowHeight));
	m_Window->InitMainWindow();
	InitializeDX();
	OnResize();

	// Create default PSO
	DXGI_FORMAT DepthFormat = DXGI_FORMAT_UNKNOWN;// TODO no depth.
	DefaultPSO.Reset(new D3D12PSO(m_Device, "TextureMap.hlsl", m_BackBufferFormat, DepthFormat, m_MsaaSampleCount, m_MsaaQuality));
}

D3D12RHI::~D3D12RHI() {
	for (uint32 i = 0; i < BACK_BUFFER_COUNT; ++i) {
		DX_RELEASE(m_SwapchainBuffer[i]);
	}
	DX_RELEASE(m_CommandAllocator);
	DX_RELEASE(m_GraphicsCommandList);
	DX_RELEASE(m_RTVDescriptorHeap);
	DX_RELEASE(m_CommandQueue);
	DX_RELEASE(m_Fence);
	DX_RELEASE(m_Swapchain);
	DX_RELEASE(m_Device);
}

void D3D12RHI::ExecuteDrawCall(GFXCmdFunc&& func) {
	// Reuse the memory associated with command recording.
	// // We can only reset when the associated command lists have finished execution on the GPU.
	DX_CHECK(m_DirectCmdListAlloc->Reset());

	// A command list can be reset after it has been added to the command queue via ExecuteCommandList.
	// Reusing the command list reuses memory.
	DX_CHECK(m_GraphicsCommandList->Reset(m_DirectCmdListAlloc, nullptr));

	m_GraphicsCommandList->RSSetViewports(1, &m_Viewport);
	m_GraphicsCommandList->RSSetScissorRects(1, &m_ScissorRect);
	// Indicate a state transition on the resource usage.
	m_GraphicsCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(BackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));
	// Clear the back buffer and depth buffer.
	float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	m_GraphicsCommandList->ClearRenderTargetView(BackBufferView(), clearColor, 0, nullptr);

	// Specify the buffers we are going to render to.
	m_GraphicsCommandList->OMSetRenderTargets(1, &BackBufferView(), true, nullptr);
	m_GraphicsCommandList->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	func(m_GraphicsCommandList);

	m_GraphicsCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(BackBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	// Done recording commands.
	DX_CHECK(m_GraphicsCommandList->Close());

	// Add the command list to the queue for execution.
	ID3D12CommandList* cmdsLists[] = { m_GraphicsCommandList };
	m_CommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// swap the back and front buffers
	DX_CHECK(m_Swapchain->Present(0, 0));
	m_CurrentBuffer = (m_CurrentBuffer + 1) % BACK_BUFFER_COUNT;

	// Wait until frame commands are complete.  This waiting is inefficient and is
	// done for simplicity.  Later we will show how to organize our rendering code
	// so we do not have to wait per frame.
	FlushCommandQueue();
}

void D3D12RHI::ImmediatelyCommit(GFXCmdFunc&& func) {
	DX_CHECK(m_DirectCmdListAlloc->Reset());
	DX_CHECK(m_GraphicsCommandList->Reset(m_DirectCmdListAlloc, nullptr));
	func(m_GraphicsCommandList);
	DX_CHECK(m_GraphicsCommandList->Close());
	ID3D12CommandList* cmdList = static_cast<ID3D12CommandList*>(m_GraphicsCommandList);
	m_CommandQueue->ExecuteCommandLists(1, &cmdList);
	FlushCommandQueue();
}

RHITextureHandle D3D12RHI::CreateTexture(uint32 width, uint32 height, uint16 layer, uint16 mip, ERHIFormat format) {
	return new D3D12Texture(m_Device, width, height, layer, mip, Convert2DXGIFormat(format));
}

void D3D12RHI::UpdateTextureData(RHITextureHandle Handle, const void* Data, uint64 ByteSize) {
	D3D12Texture* Texture = (D3D12Texture*)Handle;
	CHECK(Texture);

	ID3D12Resource* TextureResource = Texture->Resource();
	// get required buffer size
	const auto requiredSize = GetRequiredIntermediateSize(TextureResource, 0, 1);
	// create upload buffer
	ID3D12Resource* uploadBuffer;
	CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_UPLOAD);
	CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(requiredSize);
	m_Device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&uploadBuffer));

	ImmediatelyCommit([uploadBuffer, TextureResource, Data, ByteSize](ID3D12GraphicsCommandList* cmdList) {
		// copy data to upload buffer
		D3D12_SUBRESOURCE_DATA data{};
		data.pData = Data;
		data.RowPitch = TextureResource->GetDesc().Width * 4;
		data.SlicePitch = ByteSize;
		UINT64 result = UpdateSubresources(cmdList, TextureResource, uploadBuffer, 0, 0, 1, &data);
		if (FAILED(result)) {
			LOG_ERROR("UpdateSubresources failed!");
		}
		// transition state
		CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(TextureResource, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		cmdList->ResourceBarrier(1, &barrier);
	});

	DX_RELEASE(uploadBuffer);
}

void D3D12RHI::DestroyTexture(RHITextureHandle Handle) {
	D3D12Texture* Texture = (D3D12Texture*)Handle;
	delete Texture;
}

bool D3D12RHI::DrawTexture(RHITextureHandle TextureHandle) {
	if (!m_Window->Tick()) {
		return false;
	}
	D3D12Texture* Texture = (D3D12Texture*)TextureHandle;
	ExecuteDrawCall([this, Texture](ID3D12GraphicsCommandList* cmd) {
		DefaultPSO->Bind(cmd);
		Texture->BindDesc(cmd, 0);
		cmd->DrawInstanced(6, 1, 0, 0);
	});
	return true;
}

void D3D12RHI::InitializeDX() {
#if defined(DEBUG) || defined(_DEBUG) 
	// Enable the D3D12 debug layer.
	{
		ID3D12Debug* debugController;
		DX_CHECK(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
		debugController->EnableDebugLayer();
	}
#endif

	DX_CHECK(CreateDXGIFactory1(IID_PPV_ARGS(&m_DXGIFactory)));

	// Try to create hardware device.
	HRESULT hardwareResult = D3D12CreateDevice(
		nullptr,             // default adapter
		D3D_FEATURE_LEVEL_11_0,
		IID_PPV_ARGS(&m_Device));

	// Fallback to WARP device.
	if (FAILED(hardwareResult))
	{
		IDXGIAdapter* pWarpAdapter;
		DX_CHECK(m_DXGIFactory->EnumWarpAdapter(IID_PPV_ARGS(&pWarpAdapter)));

		DX_CHECK(D3D12CreateDevice(
			pWarpAdapter,
			D3D_FEATURE_LEVEL_11_0,
			IID_PPV_ARGS(&m_Device)));
	}

	DX_CHECK(m_Device->CreateFence(0, D3D12_FENCE_FLAG_NONE,
		IID_PPV_ARGS(&m_Fence)));

	m_RtvDescriptorSize = m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	//mDsvDescriptorSize = m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	//mCbvSrvUavDescriptorSize = m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// Check 4X MSAA quality support for our back buffer format.
	// All Direct3D 11 capable devices support 4X MSAA for all render 
	// target formats, so we only need to check quality support.

	D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msQualityLevels;
	msQualityLevels.Format = m_BackBufferFormat;
	msQualityLevels.SampleCount = 4;
	msQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
	msQualityLevels.NumQualityLevels = 0;
	DX_CHECK(m_Device->CheckFeatureSupport(
		D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
		&msQualityLevels,
		sizeof(msQualityLevels)));

	m_MsaaQuality = msQualityLevels.NumQualityLevels;
	ASSERT(m_MsaaQuality > 0, "Unexpected MSAA quality level.");

#ifdef _DEBUG
	LogAdapters(m_DXGIFactory, m_BackBufferFormat);
#endif

	CreateCommandObjects();
	CreateSwapchain();
	CreateInternalDescriptorHeaps();

}

void D3D12RHI::CreateCommandObjects() {
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	DX_CHECK(m_Device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_CommandQueue)));

	DX_CHECK(m_Device->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(&m_DirectCmdListAlloc)));

	DX_CHECK(m_Device->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		m_DirectCmdListAlloc, // Associated command allocator
		nullptr,                   // Initial PipelineStateObject
		IID_PPV_ARGS(&m_GraphicsCommandList)));

	// Start off in a closed state.  This is because the first time we refer 
	// to the command list we will Reset it, and it needs to be closed before
	// calling Reset.
	m_GraphicsCommandList->Close();
}


void D3D12RHI::CreateSwapchain() {
	// Release the previous swapchain we will be recreating.
	DX_RELEASE(m_Swapchain);
	DXGI_SWAP_CHAIN_DESC sd;
	sd.BufferDesc.Width = m_Window->GetWidth();
	sd.BufferDesc.Height = m_Window->GetHeight();
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferDesc.Format = m_BackBufferFormat;
	sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	sd.SampleDesc.Count = m_EnableMsaa ? m_MsaaSampleCount : 1;
	sd.SampleDesc.Quality = m_EnableMsaa ? (m_MsaaQuality - 1) : 0;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.BufferCount = BACK_BUFFER_COUNT;
	sd.OutputWindow = m_Window->GetWindow();
	sd.Windowed = true;
	sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	// Note: Swap chain uses queue to perform flush.
	DX_CHECK(m_DXGIFactory->CreateSwapChain(
		m_CommandQueue,
		&sd,
		&m_Swapchain));
}

void D3D12RHI::CreateInternalDescriptorHeaps() {
	// rtv
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
	rtvHeapDesc.NumDescriptors = BACK_BUFFER_COUNT;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvHeapDesc.NodeMask = 0;
	DX_CHECK(m_Device->CreateDescriptorHeap(
		&rtvHeapDesc, IID_PPV_ARGS(&m_RTVDescriptorHeap)));
}

void D3D12RHI::FlushCommandQueue() {
	// Advance the fence value to mark commands up to this fence point.
	m_CurrentFence++;

	// Add an instruction to the command queue to set a new fence point.  Because we 
	// are on the GPU timeline, the new fence point won't be set until the GPU finishes
	// processing all the commands prior to this Signal().
	DX_CHECK(m_CommandQueue->Signal(m_Fence, m_CurrentFence));

	// Wait until the GPU has completed commands up to this fence point.
	if (m_Fence->GetCompletedValue() < m_CurrentFence)
	{
		HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);

		// Fire event when GPU hits current fence.  
		DX_CHECK(m_Fence->SetEventOnCompletion(m_CurrentFence, eventHandle));

		// Wait until the GPU hits current fence event is fired.
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}
}

void D3D12RHI::CreateSwapchainBuffers() {
	// release previous resources
	for (uint32 i = 0; i < BACK_BUFFER_COUNT; ++i) {
		DX_RELEASE(m_SwapchainBuffer[i]);
	}
	// resize the swapchain
	DX_CHECK(m_Swapchain->ResizeBuffers(BACK_BUFFER_COUNT, m_Window->GetWidth(), m_Window->GetHeight(), m_BackBufferFormat, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));
	m_CurrentBuffer = 0;
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle{ m_RTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart() };
	for (uint32 i = 0; i < BACK_BUFFER_COUNT; ++i) {
		DX_CHECK(m_Swapchain->GetBuffer(i, IID_PPV_ARGS(&m_SwapchainBuffer[i])));
		m_Device->CreateRenderTargetView(m_SwapchainBuffer[i], nullptr, rtvHeapHandle);
		rtvHeapHandle.Offset(1, m_RtvDescriptorSize);
	}
}

D3D12_CPU_DESCRIPTOR_HANDLE D3D12RHI::BackBufferView() {
	return CD3DX12_CPU_DESCRIPTOR_HANDLE(m_RTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), (int32)m_CurrentBuffer, m_RtvDescriptorSize);
}

void D3D12RHI::BeginRender() {
	DX_CHECK(m_DirectCmdListAlloc->Reset());
	DX_CHECK(m_GraphicsCommandList->Reset(m_DirectCmdListAlloc, nullptr));
	m_GraphicsCommandList->RSSetViewports(1, &m_Viewport);
	m_GraphicsCommandList->RSSetScissorRects(1, &m_ScissorRect);
	m_GraphicsCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(BackBuffer(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));
	float colorRGBA[4]{ 0.0f, 0.0f, 0.0f, 1.0f };
	m_GraphicsCommandList->ClearRenderTargetView(BackBufferView(), colorRGBA, 0, nullptr);//clears the entire resource view
	m_GraphicsCommandList->OMSetRenderTargets(1, &BackBufferView(), true, nullptr);
}

void D3D12RHI::EndRender() {	//end and execute cmds
	//state transition
	m_GraphicsCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(BackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
	DX_CHECK(m_GraphicsCommandList->Close());

	ID3D12CommandList* cmdList = static_cast<ID3D12CommandList*>(m_GraphicsCommandList);
	m_CommandQueue->ExecuteCommandLists(1, &cmdList);

	//swap the back and front buffers
	DX_CHECK(m_Swapchain->Present(0, 0));

	m_CurrentBuffer = (m_CurrentBuffer + 1) % BACK_BUFFER_COUNT;
	// Wait until frame commands are complete.  This waiting is inefficient and is done for simplicity.
	FlushCommandQueue();
}

void D3D12RHI::OnResize() {
	CHECK(m_Device);
	CHECK(m_Swapchain);
	CHECK(m_DirectCmdListAlloc);

	// Flush before changing any resources.
	FlushCommandQueue();

	DX_CHECK(m_GraphicsCommandList->Reset(m_DirectCmdListAlloc, nullptr));

	// Release the previous resources we will be recreating.
	for (int i = 0; i < BACK_BUFFER_COUNT; ++i) {
		DX_RELEASE(m_SwapchainBuffer[i]);
	}
	// Resize the swap chain.
	DX_CHECK(m_Swapchain->ResizeBuffers(
		BACK_BUFFER_COUNT,
		m_Window->GetWidth(), m_Window->GetHeight(),
		m_BackBufferFormat,
		DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));

	m_CurrentBuffer = 0;

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(m_RTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	for (UINT i = 0; i < BACK_BUFFER_COUNT; i++)
	{
		DX_CHECK(m_Swapchain->GetBuffer(i, IID_PPV_ARGS(&m_SwapchainBuffer[i])));
		m_Device->CreateRenderTargetView(m_SwapchainBuffer[i], nullptr, rtvHeapHandle);
		rtvHeapHandle.Offset(1, m_RtvDescriptorSize);
	}

	// Execute the resize commands.
	DX_CHECK(m_GraphicsCommandList->Close());
	ID3D12CommandList* cmdsLists[] = { m_GraphicsCommandList };
	m_CommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// Wait until resize is complete.
	FlushCommandQueue();

	// Update the viewport transform to cover the client area.
	m_Viewport.TopLeftX = 0;
	m_Viewport.TopLeftY = 0;
	m_Viewport.Width = static_cast<float>(m_Window->GetWidth());
	m_Viewport.Height = static_cast<float>(m_Window->GetHeight());
	m_Viewport.MinDepth = 0.0f;
	m_Viewport.MaxDepth = 1.0f;

	m_ScissorRect = { 0, 0, static_cast<LONG>(m_Window->GetWidth()), static_cast<LONG>(m_Window->GetHeight()) };
}
