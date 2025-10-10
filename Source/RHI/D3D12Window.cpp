#include "D3D12Window.h"
#include <string>
#include "Core/Log.h"
#include <WindowsX.h>
#include <WinUser.h>
#include <wingdi.h>

// The only window
static D3D12Window* GWindow{nullptr};

LRESULT CALLBACK WindowMainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	if (!GWindow) {
		return 1;
	}
	return GWindow->MainWndProc(hwnd, msg, wParam, lParam);
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
	RECT R = { 0, 0, m_WindowWidth, m_WindowHeight };
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

D3D12Window::D3D12Window(HINSTANCE hInstance, uint32 WindowWidth, uint32 WindowHeight) {
	m_HAppInst = hInstance;
	m_WindowWidth = WindowWidth;
	m_WindowHeight = WindowHeight;
	GWindow = this;
}

D3D12Window::~D3D12Window() {
	GWindow = nullptr;
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
		m_WindowWidth = LOWORD(lParam);
		m_WindowHeight = HIWORD(lParam);
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
#pragma endregion
