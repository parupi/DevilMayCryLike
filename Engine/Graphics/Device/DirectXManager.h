#pragma once
#include <array>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include <dxcapi.h>
#include "Platform/WindowManager.h"
#include "Utility/Logger.h"
#include "Utility/StringUtility.h"
#include "DirectXTex/DirectXTex.h"
#include <vector>
#include <Math/Vector4.h>
#include "Graphics/Device/GraphicsDevice.h"
#include "Graphics/Device/CommandContext.h"
#include "Graphics/Device/SwapChainManager.h"
#include "Graphics/Device/ShaderCompiler.h"
#include "Graphics/Device/FrameTimer.h"
#include "Graphics/Resource/SrvManager.h"
#include "Graphics/Resource/RtvManager.h"
#include "Graphics/Resource/DsvManager.h"
#include "Graphics/Resource/GpuResourceFactory.h"
#include "Graphics/Resource/ResourceManager.h"

class PSOManager;

class DirectXManager
{
public:
	void Initialize(WindowManager* winManager);
	void Finalize();

	// デバイスを取得
	ID3D12Device* GetDevice() { return graphicsDevice_->GetDevice(); }
	// コマンドリストを取得
	ID3D12GraphicsCommandList* GetCommandList() const { return commandContext_->GetCommandList(); }
	// スワップチェインマネージャの取得
	SwapChainManager* GetSwapChainManager() const { return swapChainManager_.get(); }
	// バックバッファの数を取得
	size_t GetBackBufferCount() { return swapChainManager_->GetBackBufferCount(); }
	// コマンドコンテキストを取得
	CommandContext* GetCommandContext() const { return commandContext_.get(); }
	// ResourceFactoryを取得
	GpuResourceFactory* GetResourceFactory() const { return resourceFactory_.get(); }
	// Resource管理クラス取得
	ResourceManager* GetResourceManager() const { return resourceManager_.get(); }
	// RTVManagerを取得
	RtvManager* GetRtvManager() const { return rtvManager_.get(); }
	// DSVManagerを取得
	DsvManager* GetDsvManager() const { return dsvManager_.get(); }
	// SRVManagerを取得
	SrvManager* GetSrvManager() const { return srvManager_.get(); }

	// シェーダーのコンパイル（ShaderCompilerへの委譲）
	IDxcBlob* CompileShader(const std::wstring& filePath, const wchar_t* profile) {
		return shaderCompiler_->Compile(filePath, profile);
	}

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(
		D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptors, bool shaderVisible);

	Microsoft::WRL::ComPtr<ID3D12Resource> UploadTextureData(
		ID3D12Resource* texture, const DirectX::ScratchImage& mipImages);

	D3D12_RECT GetMainScissorRect() const { return scissorRect_; }
	D3D12_VIEWPORT GetMainViewport() const { return viewport_; }

	void SetMainRTV();
	void SetMainDepth(ID3D12DescriptorHeap* dsvHeap);

	void BeginDraw();
	void Render(PSOManager* psoManager, uint32_t srvIndex);
	void EndDraw();

private:
	void CreateDepthBuffer();
	void CreateRenderTargetView();
	void SetViewPort();
	void SetScissor();

	WindowManager* winManager_ = nullptr;

	std::unique_ptr<GraphicsDevice> graphicsDevice_;
	std::unique_ptr<CommandContext> commandContext_;
	std::unique_ptr<SwapChainManager> swapChainManager_;
	std::unique_ptr<GpuResourceFactory> resourceFactory_;
	std::unique_ptr<SrvManager> srvManager_;
	std::unique_ptr<RtvManager> rtvManager_;
	std::unique_ptr<DsvManager> dsvManager_;
	std::unique_ptr<ResourceManager> resourceManager_;
	std::unique_ptr<ShaderCompiler> shaderCompiler_;
	std::unique_ptr<FrameTimer> frameTimer_;

	Microsoft::WRL::ComPtr<ID3D12Resource> depthBuffer_;
	uint32_t dsvIndex_ = 0;

	D3D12_RECT scissorRect_{};
	D3D12_VIEWPORT viewport_{};

	float r = 0.6f, g = 0.5f, b = 0.1f, a = 1.0f;
};
