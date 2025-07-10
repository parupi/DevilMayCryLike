#include "LeakChecker.h"
#include <dxgidebug.h>
#include <d3d12sdklayers.h>
#include <base/DirectXManager.h>

D3DResourceLeakChecker::D3DResourceLeakChecker(DirectXManager* dxManager) : dxManager_(dxManager)
{
}

D3DResourceLeakChecker::~D3DResourceLeakChecker()
{
       // Live Object Report
    Microsoft::WRL::ComPtr<IDXGIDebug1> debug;
    if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&debug)))) {
        debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_DETAIL);
        debug->ReportLiveObjects(DXGI_DEBUG_APP, DXGI_DEBUG_RLO_DETAIL);
        debug->ReportLiveObjects(DXGI_DEBUG_D3D12, DXGI_DEBUG_RLO_DETAIL);
    }

    if (dxManager_) {
        auto device = dxManager_->GetDevice();
        if (device) {
            ULONG refCount = device->Release();
            device->AddRef();

            std::stringstream ss;
            ss << "ID3D12Device RefCount: " << refCount << "\n";
            OutputDebugStringA(ss.str().c_str());
        }
    }
}
