#include "D3DUtil.h"


HRESULT WINAPI DXTraceWDetail(_In_z_ const WCHAR* strFile, _In_ DWORD dwLine, _In_ HRESULT hr, _In_opt_ const WCHAR* strMsg){
    WCHAR strBufferLine[128];
    WCHAR strBufferError[300];
    WCHAR strBufferHR[40];
    WCHAR strBuffer[3000];

    swprintf_s(strBufferLine, 128, L"%lu", dwLine);
    if (strFile)
    {
        swprintf_s(strBuffer, 3000, L"%ls(%ls): ", strFile, strBufferLine);
        OutputDebugStringW(strBuffer);
    }

    size_t nMsgLen = (strMsg) ? wcsnlen_s(strMsg, 1024) : 0;
    if (nMsgLen > 0)
    {
        OutputDebugStringW(strMsg);
        OutputDebugStringW(L" ");
    }
    // Windows SDK 8.0��DirectX�Ĵ�����Ϣ�Ѿ����ɽ��������У�����ͨ��FormatMessageW��ȡ������Ϣ�ַ���
    // ����Ҫ�����ַ����ڴ�
    FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr, hr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        strBufferError, 256, nullptr);

    WCHAR* errorStr = wcsrchr(strBufferError, L'\r');
    if (errorStr) errorStr[0] = L'\0';    // ����FormatMessageW�����Ļ��з�(��\r\n��\r�û�Ϊ\0����)

    swprintf_s(strBufferHR, 40, L" (0x%0.8x)", hr);
    wcscat_s(strBufferError, strBufferHR);
    swprintf_s(strBuffer, 3000, L"�����뺬�壺%ls", strBufferError);
    OutputDebugStringW(strBuffer);

    OutputDebugStringW(L"\n");

    return hr;
}

void WINAPI DXTraceW(_In_z_ const WCHAR* strFile, _In_ DWORD dwLine, _In_ HRESULT hr, _In_opt_ const WCHAR* strMsg) {
    WCHAR e[256];
    FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, hr, MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT), e, 256, nullptr); \
        std::wprintf(L"[DX_ERROR]%ls: %lu 0x%0.8x, %ls", strFile, dwLine, hr, e);
}