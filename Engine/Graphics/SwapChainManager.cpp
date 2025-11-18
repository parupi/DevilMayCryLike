#include "SwapChainManager.h"
#include <base/WindowManager.h>
#include "GraphicsDevice.h"
#include "CommandContext.h"
#include <base/Logger.h>
#include <cassert>

SwapChainManager::~SwapChainManager()
{
	for (auto& buffer : backBuffers_) {
		buffer.Reset();
	}
	backBuffers_.clear();
	swapChain_.Reset();
}

bool SwapChainManager::Initialize(GraphicsDevice* device, CommandContext* commandContext, WindowManager* windowManager)
{
	if (!device || !commandContext || !windowManager) return false;

    windowManager_ = windowManager;
	graphicsDevice_ = device;
	commandContext_ = commandContext;

    // swapChainの生成
    if (!CreateSwapChain()) return false;
    // バックバッファの生成
    if (!RetrieveBackBuffers()) return false;

	return true;
}

void SwapChainManager::Present()
{
    Logger::CheckHR(swapChain_->Present(1, 0), "failed present");
}

UINT SwapChainManager::GetCurrentBackBufferIndex() const
{
    return swapChain_->GetCurrentBackBufferIndex();
}

ID3D12Resource* SwapChainManager::GetBackBuffer(UINT index) const  
{  
    assert(static_cast<size_t>(index) < backBuffers_.size());
   return backBuffers_[index].Get();  
}

size_t SwapChainManager::GetBackBufferCount() const
{
    return backBuffers_.size();
}

bool SwapChainManager::CreateSwapChain()
{
    swapChainDesc_.Width = WindowManager::kClientWidth;
    swapChainDesc_.Height = WindowManager::kClientHeight;
    swapChainDesc_.Format = format_;
    swapChainDesc_.SampleDesc.Count = 1;
    swapChainDesc_.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc_.BufferCount = 2;
    swapChainDesc_.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc_.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    swapChainDesc_.Scaling = DXGI_SCALING_STRETCH;
    swapChainDesc_.Stereo = FALSE;

    HRESULT hr = graphicsDevice_->GetFactory()->CreateSwapChainForHwnd(
        reinterpret_cast<IUnknown*>(commandContext_->GetCommandQueue()),
        windowManager_->GetHwnd(),
        &swapChainDesc_,
        nullptr,
        nullptr,
        reinterpret_cast<IDXGISwapChain1**>(swapChain_.GetAddressOf())
    );

    if (FAILED(hr)) {
        Logger::Log("Failed to create SwapChain.\n");
        return false;
    }

    Logger::Log("Successfully created SwapChain.\n");
    return true;
}


bool SwapChainManager::RetrieveBackBuffers()
{
    backBuffers_.resize(swapChainDesc_.BufferCount);

    for (UINT i = 0; i < swapChainDesc_.BufferCount; ++i) {
        Logger::CheckHR(swapChain_->GetBuffer(i, IID_PPV_ARGS(&backBuffers_[i])), "");
        std::wstring name = L"BackBuffer" + std::to_wstring(i);
        backBuffers_[i]->SetName(name.c_str());
    }

    return true;
}
