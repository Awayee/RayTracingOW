#include "D3D12App.h"
#include <WindowsX.h>
#include <string>

std::unique_ptr<D3D12App> D3D12App::s_Instance{};

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


LRESULT CALLBACK WindowMainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	if(!D3D12Window::Instance()) {
		return 1;
	}
	return D3D12Window::Instance()->MainWndProc(hwnd, msg, wParam, lParam);
}

bool D3D12Window::InitMainWindow() {
	WNDCLASS wc;
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WindowMainWndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = m_HAppInst;
	wc.hIcon = LoadIcon(0, IDI_APPLICATION);
	wc.hCursor = LoadCursor(0, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
	wc.lpszMenuName = 0;
	wc.lpszClassName = PROJECT_NAME;

	if (!RegisterClass(&wc)) {
		MessageBox(0, "RegisterClass Failed.", 0, 0);
		return false;
	}

	// Compute window rectangle dimensions based on requested client area dimensions.
	RECT R = { 0, 0, m_WindowSize.X, m_WindowSize.Y };
	AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, false);
	int width = R.right - R.left;
	int height = R.bottom - R.top;
	m_MainWnd = CreateWindow(PROJECT_NAME, PROJECT_NAME, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, width, height, 0, 0, m_HAppInst, 0);
	if (!m_MainWnd) {
		MessageBox(0, "CreateWindow Failed.", 0, 0);
		ASSERT(0, "");
		return false;
	}
	ShowWindow(m_MainWnd, SW_SHOW);
	UpdateWindow(m_MainWnd);

	return true;
}

bool D3D12Window::Tick() {
	MSG msg = { 0 };
	if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	if (msg.message != WM_QUIT && msg.message != WM_CLOSE) {
		return true;
	}
	return false;
}

D3D12Window::D3D12Window(HINSTANCE hInstance, Math::USize size) {
	m_HAppInst = hInstance;
	m_WindowSize = size;
}

D3D12Window::~D3D12Window() {
}

LRESULT D3D12Window::MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg){
		// WM_ACTIVATE is sent when the window is activated or deactivated.  
		// We pause the game when the window is deactivated and unpause it 
		// when it becomes active.  
	case WM_ACTIVATE:
		if (LOWORD(wParam) == WA_INACTIVE){
			m_AppPaused = true;
		}
		else{
			m_AppPaused = false;
		}
		return 0;

		// WM_SIZE is sent when the user resizes the window.  
	case WM_SIZE:
		// Save the new client area dimensions.
		m_WindowSize.X = LOWORD(lParam);
		m_WindowSize.Y = HIWORD(lParam);
		if (wParam == SIZE_MINIMIZED){
			m_AppPaused = true;
			m_Minimized = true;
			m_Maximized = false;
		}
		else if (wParam == SIZE_MAXIMIZED){
			m_AppPaused = false;
			m_Minimized = false;
			m_Maximized = true;
			OnResize();
		}
		else if (wParam == SIZE_RESTORED){

			// Restoring from minimized state?
			if (m_Minimized){
				m_AppPaused = false;
				m_Minimized = false;
				OnResize();
			}

			// Restoring from maximized state?
			else if (m_Maximized){
				m_AppPaused = false;
				m_Maximized = false;
				OnResize();
			}
			else if (m_Resizing){
				// If user is dragging the resize bars, we do not resize 
				// the buffers here because as the user continuously 
				// drags the resize bars, a stream of WM_SIZE messages are
				// sent to the window, and it would be pointless (and slow)
				// to resize for each WM_SIZE message received from dragging
				// the resize bars.  So instead, we reset after the user is 
				// done resizing the window and releases the resize bars, which 
				// sends a WM_EXITSIZEMOVE message.
			}// API call such as SetWindowPos or m_Swapchain->SetFullscreenState.
			else {
				OnResize();
			}
		}
		return 0;

		// WM_EXITSIZEMOVE is sent when the user grabs the resize bars.
	case WM_ENTERSIZEMOVE:
		m_AppPaused = true;
		m_Resizing = true;
		return 0;

		// WM_EXITSIZEMOVE is sent when the user releases the resize bars.
		// Here we reset everything based on the new window dimensions.
	case WM_EXITSIZEMOVE:
		m_AppPaused = false;
		m_Resizing = false;
		OnResize();
		return 0;

		// WM_DESTROY is sent when the window is being destroyed.
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;

		// The WM_MENUCHAR message is sent when a menu is active and the user presses 
		// a key that does not correspond to any mnemonic or accelerator key. 
	case WM_MENUCHAR:
		// Don't beep when we alt-enter.
		return MAKELRESULT(0, MNC_CLOSE);

		// Catch this message so to prevent the window from becoming too small.
	case WM_GETMINMAXINFO:
		((MINMAXINFO*)lParam)->ptMinTrackSize.x = 200;
		((MINMAXINFO*)lParam)->ptMinTrackSize.y = 200;
		return 0;

	case WM_LBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN:
		OnMouseDown(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
	case WM_LBUTTONUP:
	case WM_MBUTTONUP:
	case WM_RBUTTONUP:
		OnMouseUp(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
	case WM_MOUSEMOVE:
		OnMouseMove(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}

void D3D12Window::OnMouseDown(WPARAM btnState, int x, int y) {
}

void D3D12Window::OnMouseUp(WPARAM btnState, int x, int y) {
}

void D3D12Window::OnMouseMove(WPARAM btnState, int x, int y) {
}

void D3D12Window::OnResize() {
}

D3D12App* D3D12App::Instance() {
	return s_Instance.get();
}

void D3D12App::Initialize() {
	s_Instance.reset(new D3D12App());
}

void D3D12App::Release() {
	s_Instance.reset();
}

void D3D12App::ExecuteDrawCall(GFXCmdFunc&& func) {
	// Reuse the memory associated with command recording.
// We can only reset when the associated command lists have finished execution on the GPU.
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

void D3D12App::ImmediatelyCommit(GFXCmdFunc&& func) {
	DX_CHECK(m_DirectCmdListAlloc->Reset());
	DX_CHECK(m_GraphicsCommandList->Reset(m_DirectCmdListAlloc, nullptr));
	func(m_GraphicsCommandList);
	DX_CHECK(m_GraphicsCommandList->Close());
	ID3D12CommandList* cmdList = static_cast<ID3D12CommandList*>(m_GraphicsCommandList);
	m_CommandQueue->ExecuteCommandLists(1, &cmdList);
	FlushCommandQueue();
}

D3D12App::D3D12App() {
	InitializeDX();
	OnResize();
}

D3D12App::~D3D12App() {
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

void D3D12App::InitializeDX() {
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
	ASSERT(m_MsaaQuality > 0 && "Unexpected MSAA quality level.");

#ifdef _DEBUG
	LogAdapters(m_DXGIFactory, m_BackBufferFormat);
#endif

	CreateCommandObjects();
	CreateSwapchain();
	CreateInternalDescriptorHeaps();

}

void D3D12App::CreateCommandObjects() {
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


void D3D12App::CreateSwapchain() {
	// Release the previous swapchain we will be recreating.
	DX_RELEASE(m_Swapchain);
	Math::USize windowSize = D3D12Window::Instance()->GetWindowSize();
	HWND wnd = D3D12Window::Instance()->GetWindow();
	DXGI_SWAP_CHAIN_DESC sd;
	sd.BufferDesc.Width = windowSize.X;
	sd.BufferDesc.Height = windowSize.Y;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferDesc.Format = m_BackBufferFormat;
	sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	sd.SampleDesc.Count = m_EnableMsaa ? m_MsaaSampleCount : 1;
	sd.SampleDesc.Quality = m_EnableMsaa ? (m_MsaaQuality - 1) : 0;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.BufferCount = BACK_BUFFER_COUNT;
	sd.OutputWindow = wnd;
	sd.Windowed = true;
	sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	// Note: Swap chain uses queue to perform flush.
	DX_CHECK(m_DXGIFactory->CreateSwapChain(
		m_CommandQueue,
		&sd,
		&m_Swapchain));
}

void D3D12App::CreateInternalDescriptorHeaps() {
	// rtv
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
	rtvHeapDesc.NumDescriptors = BACK_BUFFER_COUNT;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvHeapDesc.NodeMask = 0;
	DX_CHECK(m_Device->CreateDescriptorHeap(
		&rtvHeapDesc, IID_PPV_ARGS(&m_RTVDescriptorHeap)));
}

void D3D12App::FlushCommandQueue() {
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

void D3D12App::CreateSwapchainBuffers() {
	// release previous resources
	for (uint32 i = 0; i < BACK_BUFFER_COUNT; ++i) {
		DX_RELEASE(m_SwapchainBuffer[i]);
	}
	Math::USize windowSize = D3D12Window::Instance()->GetWindowSize();
	// resize the swapchain
	DX_CHECK(m_Swapchain->ResizeBuffers(BACK_BUFFER_COUNT, windowSize.X, windowSize.Y, m_BackBufferFormat, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));
	m_CurrentBuffer = 0;
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle{ m_RTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart() };
	for (uint32 i = 0; i < BACK_BUFFER_COUNT; ++i) {
		DX_CHECK(m_Swapchain->GetBuffer(i, IID_PPV_ARGS(&m_SwapchainBuffer[i])));
		m_Device->CreateRenderTargetView(m_SwapchainBuffer[i], nullptr, rtvHeapHandle);
		rtvHeapHandle.Offset(1, m_RtvDescriptorSize);
	}
}

D3D12_CPU_DESCRIPTOR_HANDLE D3D12App::BackBufferView() {
	return CD3DX12_CPU_DESCRIPTOR_HANDLE(m_RTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), (int32)m_CurrentBuffer, m_RtvDescriptorSize);
}

void D3D12App::BeginRender() {
	DX_CHECK(m_DirectCmdListAlloc->Reset());
	DX_CHECK(m_GraphicsCommandList->Reset(m_DirectCmdListAlloc, nullptr));
	m_GraphicsCommandList->RSSetViewports(1, &m_Viewport);
	m_GraphicsCommandList->RSSetScissorRects(1, &m_ScissorRect);
	m_GraphicsCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(BackBuffer(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));
	float colorRGBA[4]{ 0.0f, 0.0f, 0.0f, 1.0f };
	m_GraphicsCommandList->ClearRenderTargetView(BackBufferView(), colorRGBA, 0, nullptr);//clears the entire resource view
	m_GraphicsCommandList->OMSetRenderTargets(1, &BackBufferView(), true, nullptr);
}

void D3D12App::EndRender() {	//end and execute cmds
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

void D3D12App::OnResize() {
	ASSERT(m_Device);
	ASSERT(m_Swapchain);
	ASSERT(m_DirectCmdListAlloc);

	// Flush before changing any resources.
	FlushCommandQueue();

	DX_CHECK(m_GraphicsCommandList->Reset(m_DirectCmdListAlloc, nullptr));

	// Release the previous resources we will be recreating.
	for (int i = 0; i < BACK_BUFFER_COUNT; ++i) {
		DX_RELEASE(m_SwapchainBuffer[i]);
	}

	Math::USize windowSize = D3D12Window::Instance()->GetWindowSize();
	// Resize the swap chain.
	DX_CHECK(m_Swapchain->ResizeBuffers(
		BACK_BUFFER_COUNT,
		windowSize.X, windowSize.Y,
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
	m_Viewport.Width = static_cast<float>(windowSize.X);
	m_Viewport.Height = static_cast<float>(windowSize.Y);
	m_Viewport.MinDepth = 0.0f;
	m_Viewport.MaxDepth = 1.0f;

	m_ScissorRect = { 0, 0, static_cast<LONG>(windowSize.X), static_cast<LONG>(windowSize.Y) };
}
