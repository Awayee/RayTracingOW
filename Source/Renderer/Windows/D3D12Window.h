#pragma once
#include "Core/Defines.h"
#include <Windows.h>
#include <windef.h>

class D3D12Window {
public:
	D3D12Window(HINSTANCE hInstance, uint32 WindowWidth, uint32 WindowHeight);
	~D3D12Window();
	uint32 GetWidth() const {return m_WindowWidth;}
	uint32 GetHeight() const {return m_WindowHeight;}
	HWND GetWindow() { return m_MainWnd; }
	LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam); //window loop func
	bool InitMainWindow();
	bool Tick();
private:
	HINSTANCE m_HAppInst{ nullptr };
	HWND m_MainWnd{ 0 };
	bool m_AppPaused{ false };
	bool m_Minimized{ false };
	bool m_Maximized{ false };
	bool m_Resizing{ false };
	uint32 m_WindowWidth;
	uint32 m_WindowHeight;
	friend LRESULT WindowMainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	void OnMouseDown(WPARAM btnState, int x, int y);
	void OnMouseUp(WPARAM btnState, int x, int y);
	void OnMouseMove(WPARAM btnState, int x, int y);
	void OnResize();
};
