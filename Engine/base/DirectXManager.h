#pragma once
#include <array>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include <dxcapi.h>
#include "WindowManager.h"
#include "Logger.h"
#include "StringUtility.h"
#include "DirectXTex/DirectXTex.h"
#include <chrono>
#include <thread>
#include <vector>
#include <math/Vector4.h>
#include "Graphics/GraphicsDevice.h"
#include "Graphics/CommandContext.h"
#include "Graphics/SwapChainManager.h"
#include <base/SrvManager.h>
#include <base/RtvManager.h>
#include "DsvManager.h"

class DirectXManager
{
public:
	// 初期化
	void Initialize(WindowManager* winManager);
	// 終了
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
	// RTVManagerを取得
	RtvManager* GetRtvManager()const { return rtvManager_.get(); }
	// DSVManagerを取得
	DsvManager* GetDsvManager()const { return dsvManager_.get(); }
	// SRVManagerを取得
	SrvManager* GetSrvManager()const { return srvManager_.get(); }
private: // メンバ変数
	// WindowAPI
	WindowManager* winManager_ = nullptr;

	std::unique_ptr<GraphicsDevice> graphicsDevice_ = nullptr;

	std::unique_ptr<CommandContext> commandContext_ = nullptr;

	std::unique_ptr<SwapChainManager> swapChainManager_ = nullptr;

	std::unique_ptr<SrvManager> srvManager_ = nullptr;

	std::unique_ptr<RtvManager> rtvManager_ = nullptr;

	std::unique_ptr<DsvManager> dsvManager_ = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> depthBuffer_;

	// シザー矩形
	D3D12_RECT scissorRect_{};
	// ビューポート
	D3D12_VIEWPORT viewport_{};
	// DXC関連の宣言
	Microsoft::WRL::ComPtr<IDxcUtils> dxcUtils_ = nullptr;
	Microsoft::WRL::ComPtr<IDxcCompiler3> dxcCompiler_ = nullptr;
	Microsoft::WRL::ComPtr<IDxcIncludeHandler> includeHandler_ = nullptr;

	//GBuffer gBuffer_;

	// 記録時間(FPS固定用)
	std::chrono::steady_clock::time_point reference_;

	// クリアカラーを設定
	float r = 0.6f, g = 0.5f, b = 0.1f, a = 1.0f;
	

private:
	void InitializeFixFPS();

	void UpdateFixFPS();

	void CreateDepthBuffer();

	void CreateRenderTargetView();

	void SetViewPort();

	void SetScissor();

	void InitializeDXCCompiler();
public:
	void SetMainRTV();

	void SetMainDepth(ID3D12DescriptorHeap* dsvHeap);


	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptors, bool shaderVisible);

	IDxcBlob* CompileShader(const std::wstring& filePath, const wchar_t* profile);
	Microsoft::WRL::ComPtr<ID3D12Resource> CreateDepthStencilTextureResource(uint32_t width, uint32_t height, DXGI_FORMAT format);
	Microsoft::WRL::ComPtr<ID3D12Resource> CreateTextureResource(const DirectX::TexMetadata& metadata);

	Microsoft::WRL::ComPtr<ID3D12Resource> UploadTextureData(ID3D12Resource* texture, const DirectX::ScratchImage& mipImages, ID3D12Device* device, ID3D12GraphicsCommandList* commandList);

	void CreateBufferResource(size_t sizeInBytes, Microsoft::WRL::ComPtr<ID3D12Resource>& outResource, bool isUAV = false);

	// オフスクリーン用関数
	Microsoft::WRL::ComPtr<ID3D12Resource> CreateRenderTextureResource(uint32_t width, uint32_t height, DXGI_FORMAT format);
	// GBuffer用のリソース生成関数
	Microsoft::WRL::ComPtr<ID3D12Resource> CreateGBufferResource(UINT width, UINT height, D3D12_CLEAR_VALUE clear);

	// シザー矩形
	D3D12_RECT GetMainScissorRect() const { return scissorRect_; }
	// ビューポート
	D3D12_VIEWPORT GetMainViewport() const { return viewport_; }

	/// <summary>
	///	描画前処理
	/// </summary>
	void BeginDraw();


	/// <summary>
	/// 描画後処理
	/// </summary>
	void EndDraw();

	void FlushUpload();
};