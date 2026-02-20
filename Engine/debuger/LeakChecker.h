#pragma once
#include <wrl.h>
#include <dxgidebug.h>
#include <d3d12sdklayers.h>
#include <sstream>
#include "Graphics/Device/DirectXManager.h"

class D3DResourceLeakChecker {
public:
    D3DResourceLeakChecker() = default;
    ~D3DResourceLeakChecker();

    // チェックを明示的に実行
    void Check();
};
