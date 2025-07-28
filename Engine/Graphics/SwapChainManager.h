#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include <vector>

class WindowManager;
class CommandContext;
class GraphicsDevice;

class SwapChainManager
{
public:
	SwapChainManager() = default;
	~SwapChainManager();

	bool Initialize(GraphicsDevice* device, CommandContext* commandContext, WindowManager* windowManager);

	void Present();


	UINT GetCurrentBackBufferIndex() const;
	ID3D12Resource* GetBackBuffer(UINT index) const;
	size_t GetBackBufferCount() const;
	DXGI_FORMAT GetFormat() const { return format_; }
private:
	bool CreateSwapChain();
	bool RetrieveBackBuffers();

private:
	WindowManager* windowManager_ = nullptr;
	GraphicsDevice* graphicsDevice_ = nullptr;
	CommandContext* commandContext_ = nullptr;

	Microsoft::WRL::ComPtr<IDXGISwapChain4> swapChain_ = nullptr;
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc_{};

	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> backBuffers_;
	DXGI_FORMAT format_ = DXGI_FORMAT_R8G8B8A8_UNORM;
};

