#include "LeakChecker.h"
#include <Windows.h>

D3DResourceLeakChecker::~D3DResourceLeakChecker()
{
    Check();
}

void D3DResourceLeakChecker::Check()
{
    OutputDebugStringA("---- D3DResourceLeakChecker Start ----\n");

    // Live Object Report 出力
    Microsoft::WRL::ComPtr<IDXGIDebug1> debug;
    if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&debug)))) {
        debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_DETAIL);
        debug->ReportLiveObjects(DXGI_DEBUG_APP, DXGI_DEBUG_RLO_DETAIL);
        debug->ReportLiveObjects(DXGI_DEBUG_D3D12, DXGI_DEBUG_RLO_DETAIL);
    }

    OutputDebugStringA("---- D3DResourceLeakChecker End ----\n");
}
