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

class SrvManager;


// DxManager.h
struct GBuffer {
	Microsoft::WRL::ComPtr<ID3D12Resource> albedo;
	Microsoft::WRL::ComPtr<ID3D12Resource> normal;
	Microsoft::WRL::ComPtr<ID3D12Resource> depth;

	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[2]; // Albedo, Normal
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE srvHandles[3]; // For lighting pass later
};

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
	ID3D12GraphicsCommandList* GetCommandList() { return commandContext_->GetCommandList(); }
	// バックバッファの数を取得
	size_t GetBackBufferCount() { return swapChainManager_->GetBackBufferCount(); }

	CommandContext* GetCommandContext() const { return commandContext_.get(); }
private: // メンバ変数
	// WindowAPI
	WindowManager* winManager_ = nullptr;

	std::unique_ptr<GraphicsDevice> graphicsDevice_ = nullptr;

	std::unique_ptr<CommandContext> commandContext_ = nullptr;

	std::unique_ptr<SwapChainManager> swapChainManager_ = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> depthBuffer_;

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvHeap_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvHeapGBuffer_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvHeap_ = nullptr;


	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc_{};

	static inline uint32_t descriptorSizeRTV_;
	static inline uint32_t descriptorSizeDSV_;
	// シザー矩形
	D3D12_RECT scissorRect_{};
	// ビューポート
	D3D12_VIEWPORT viewport_{};
	// DXC関連の宣言
	Microsoft::WRL::ComPtr<IDxcUtils> dxcUtils_ = nullptr;
	Microsoft::WRL::ComPtr<IDxcCompiler3> dxcCompiler_ = nullptr;
	Microsoft::WRL::ComPtr<IDxcIncludeHandler> includeHandler_ = nullptr;

	// RTVを2つ作るのでディスクリプタを2つ用意
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles_[6]{};

	std::unordered_map<ID3D12Resource*, D3D12_CPU_DESCRIPTOR_HANDLE> rtvHandleMap_;
	std::unordered_map<ID3D12Resource*, D3D12_CPU_DESCRIPTOR_HANDLE> dsvHandleMap_;

	UINT rtvDescriptorSize_ = 0;
	UINT currentRTVIndex_ = 0;

	//UINT rtvDescriptorSize_ = 0;
	UINT currentGBufferRTVIndex_ = 0;

	UINT dsvDescriptorSize_ = 0;
	UINT currentDSVIndex_ = 0;

	GBuffer gBuffer_;

	//D3D12_RESOURCE_BARRIER barrier_{};

	// 記録時間(FPS固定用)
	std::chrono::steady_clock::time_point reference_;

	// オフスクリーン用変数
	//Microsoft::WRL::ComPtr<ID3D12Resource> offScreenResource_;
	//uint32_t srvIndex_;
	//std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> srvHandle_;

	D3D12_CLEAR_VALUE clearValue{};


private:

	void InitializeFixFPS();
	void UpdateFixFPS();
public:

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(Microsoft::WRL::ComPtr<ID3D12Device> device, D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptors, bool shaderVisible);

	IDxcBlob* CompileShader(const std::wstring& filePath, const wchar_t* profile);
	Microsoft::WRL::ComPtr<ID3D12Resource> CreateDepthStencilTextureResource(uint32_t width, uint32_t height, DXGI_FORMAT format);
	Microsoft::WRL::ComPtr<ID3D12Resource> CreateTextureResource(const DirectX::TexMetadata& metadata);

	Microsoft::WRL::ComPtr<ID3D12Resource> UploadTextureData(ID3D12Resource* texture, const DirectX::ScratchImage& mipImages, ID3D12Device* device, ID3D12GraphicsCommandList* commandList);

	void CreateBufferResource(size_t sizeInBytes, Microsoft::WRL::ComPtr<ID3D12Resource>& outResource, bool isUAV = false);

	// オフスクリーン用関数
	Microsoft::WRL::ComPtr<ID3D12Resource> CreateRenderTextureResource(uint32_t width, uint32_t height, DXGI_FORMAT format);
	//void CreateRTVForOffScreen();
	//void CreateSRVForOffScreen(SrvManager* srvManager);
	////std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> GetSrvHandle() const { return srvHandle_; }

	D3D12_CPU_DESCRIPTOR_HANDLE CreateRTVForTexture(ID3D12Resource* resource, DXGI_FORMAT format);

	void SetRenderTarget(D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle);

	D3D12_CPU_DESCRIPTOR_HANDLE GetDSVHandle() const { return dsvHeap_->GetCPUDescriptorHandleForHeapStart(); }

	// ディファ―ドレンダリング用のリソース作成
	void CreateGBuffer(UINT width, UINT height);
	// rtvのアロケート
	D3D12_CPU_DESCRIPTOR_HANDLE AllocateNextRTVHandle();
	D3D12_CPU_DESCRIPTOR_HANDLE AllocateNextGBufferRTVHandle();
	// dsvのアロケート
	D3D12_CPU_DESCRIPTOR_HANDLE AllocateNextDSVHandle();

	// ディファ―ドレンダリング用のリソースハンドルを取得
	D3D12_CPU_DESCRIPTOR_HANDLE GetRTV(ID3D12Resource* resource);
	D3D12_CPU_DESCRIPTOR_HANDLE GetDSV(ID3D12Resource* resource);

	const GBuffer& GetGBuffer() const { return gBuffer_; }
private:
	// スワップチェーンの生成


	void CreateDepthBuffer();

	void CreateHeap();

	void CreateRenderTargetView();

	void InitializeDepthStencilView();

	//void CreateFence();

	void SetViewPort();

	void SetScissor();

	void InitializeDXCCompiler();


public:
	/// <summary>
	///	描画前処理
	/// </summary>
	void BeginDraw();

	void BeginDrawForRenderTarget();

	/// <summary>
	/// 描画後処理
	/// </summary>
	void EndDraw();

	void FlushUpload();

public: // ゲッター/セッター //
	uint32_t GetDescriptorSizeRTV() { return descriptorSizeRTV_; }
	uint32_t GetDescriptorSizeDSV() { return descriptorSizeDSV_; }
};