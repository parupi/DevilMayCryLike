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
#include "GpuResourceFactory.h"
#include "ResourceManager.h"

class PSOManager;

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
	// ResourceFactoryを取得
	GpuResourceFactory* GetResourceFactory() const { return resourceFactory_.get(); }
	// Resource管理クラス取得
	ResourceManager* GetResourceManager() const { return resourceManager_.get(); }
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

	std::unique_ptr<GpuResourceFactory> resourceFactory_ = nullptr;

	std::unique_ptr<SrvManager> srvManager_ = nullptr;

	std::unique_ptr<RtvManager> rtvManager_ = nullptr;

	std::unique_ptr<DsvManager> dsvManager_ = nullptr;

	std::unique_ptr<ResourceManager> resourceManager_ = nullptr;

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
	
	// DirectXManager.h（メンバ例）
	static const int kNumFramesInFlight = 3;
	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> pendingRelease_[kNumFramesInFlight];
	uint64_t frameFenceValues_[kNumFramesInFlight]; // フレームごとの fence 値を保存
	int currentFrameIndex_ = 0;

	// pending release 管理（fence と組で保持）
	std::vector<std::pair<uint64_t, Microsoft::WRL::ComPtr<ID3D12Resource>>> pendingReleases_;

	// staging（Material::~Material などから登録される一時キュー）
	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> pendingReleaseStaging_;
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
	//Microsoft::WRL::ComPtr<ID3D12Resource> CreateDepthStencilTextureResource(uint32_t width, uint32_t height, DXGI_FORMAT format);
	//Microsoft::WRL::ComPtr<ID3D12Resource> CreateTextureResource(const DirectX::TexMetadata& metadata);

	Microsoft::WRL::ComPtr<ID3D12Resource> UploadTextureData(ID3D12Resource* texture, const DirectX::ScratchImage& mipImages);

	//void CreateBufferResource(size_t sizeInBytes, Microsoft::WRL::ComPtr<ID3D12Resource>& outResource, bool isUAV = false);

	void RegisterResourceForRelease(Microsoft::WRL::ComPtr<ID3D12Resource> res);

	void OnBeginFrame();

	void MoveStagingToPending(uint64_t fenceValue);

	void ProcessPendingReleases();

	std::wstring GetResourceDebugName(ID3D12Resource* resource);

	// シザー矩形
	D3D12_RECT GetMainScissorRect() const { return scissorRect_; }
	// ビューポート
	D3D12_VIEWPORT GetMainViewport() const { return viewport_; }

	/// <summary>
	///	描画前処理
	/// </summary>
	void BeginDraw();

	// 最終的な描画実行
	void Render(PSOManager* psoManager, uint32_t srvIndex);

	/// <summary>
	/// 描画後処理
	/// </summary>
	void EndDraw();

	void FlushUpload();
};