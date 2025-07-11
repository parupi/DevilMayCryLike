#pragma once
#include <wrl.h>
#include <dxgidebug.h>
#include <d3d12sdklayers.h>
#include <sstream>
#include <base/DirectXManager.h>

class D3DResourceLeakChecker {
public:
    D3DResourceLeakChecker() = default;
    ~D3DResourceLeakChecker() = default;

    // DXManager を後から渡す
    void SetDXManager(DirectXManager* dxManager);

    // チェックを明示的に実行
    void Check();

private:
    DirectXManager* dxManager_ = nullptr;
};
