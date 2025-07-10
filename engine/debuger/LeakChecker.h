#pragma once
#include <wrl.h>
#include <dxgi1_3.h>

class DirectXManager;

struct D3DResourceLeakChecker {
    D3DResourceLeakChecker(DirectXManager* dxManager);
    ~D3DResourceLeakChecker();

private:
    DirectXManager* dxManager_ = nullptr;
};