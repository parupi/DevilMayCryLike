#include "LeakChecker.h"
#include <Windows.h>

D3DResourceLeakChecker::~D3DResourceLeakChecker()
{
    Check();
}

void D3DResourceLeakChecker::SetDXManager(DirectXManager* dxManager)
{
    dxManager_ = dxManager;
}

void D3DResourceLeakChecker::Check()
{
    OutputDebugStringA("---- D3DResourceLeakChecker Start ----\n");

    // まずは RefCount を確認
    if (dxManager_) {
        auto device = dxManager_->GetDevice();
        if (device) {
            device->AddRef(); // 一時的に加算してから Release で現在のカウントを取得
            ULONG refCount = device->Release();

            std::stringstream ss;
            ss << "[LeakChecker] ID3D12Device RefCount: " << refCount << "\n";
            OutputDebugStringA(ss.str().c_str());
        }
    }

    // Live Object Report 出力
    Microsoft::WRL::ComPtr<IDXGIDebug1> debug;
    if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&debug)))) {
        debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_DETAIL);
        debug->ReportLiveObjects(DXGI_DEBUG_APP, DXGI_DEBUG_RLO_DETAIL);
        debug->ReportLiveObjects(DXGI_DEBUG_D3D12, DXGI_DEBUG_RLO_DETAIL);
    }

    OutputDebugStringA("---- D3DResourceLeakChecker End ----\n");
}
