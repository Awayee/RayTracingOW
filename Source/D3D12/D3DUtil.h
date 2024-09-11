#pragma once
#include <d3d12.h>
#define D3DX12_NO_STATE_OBJECT_HELPERS
#define D3DX12_NO_CHECK_FEATURE_SUPPORT_CLASS
#include <d3dx12.h>
#include <dxgi1_4.h>
#include <d3dcompiler.h>
#include <vector>
#include <memory>
#include <iostream>
#if defined(DEBUG) || defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#define LOG(...) printf(__VA_ARGS__); std::cout<<std::endl

#define ASSERT(x, s) if(!(x))throw s


HRESULT WINAPI DXTraceWDetail(_In_z_ const WCHAR * strFile, _In_ DWORD dwLine, _In_ HRESULT hr, _In_opt_ const WCHAR * strMsg);

void WINAPI DXTraceW(_In_z_ const WCHAR * strFile, _In_ DWORD dwLine, _In_ HRESULT hr, _In_opt_ const WCHAR * strMsg);

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

#define DX_RELEASE(x) if(x)(x)->Release(); (x)=nullptr

inline UINT CalcConstantBufferByteSize(UINT byteSize)
{
    // Constant buffers must be a multiple of the minimum hardware
    // allocation size (usually 256 bytes).  So round up to nearest
    // multiple of 256.  We do this by adding 255 and then masking off
    // the lower 2 bytes which store all bits < 256.
    // Example: Suppose byteSize = 300.
    // (300 + 255) & ~255
    // 555 & ~255
    // 0x022B & ~0x00ff
    // 0x022B & 0xff00
    // 0x0200
    // 512
    return (byteSize + 255) & ~255;
}