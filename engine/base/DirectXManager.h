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

class SrvManager;

class DirectXManager
{
public:
	// 初期化
	void Initialize(WindowManager* winManager);
	// 終了
	void Finalize();

private: // メンバ変数
	// WindowAPI
	WindowManager* winManager_ = nullptr;

	Microsoft::WRL::ComPtr<IDXGIFactory7> dxgiFactory_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Device> device_ = nullptr;

	Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList_ = nullptr;

	Microsoft::WRL::ComPtr<IDXGISwapChain4> swapChain_ = nullptr;
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> backBuffers_;

	Microsoft::WRL::ComPtr<ID3D12Resource> depthBuffer_;

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvHeap_ = nullptr;
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
	uint64_t fenceValue_ = 0;
	// TransitionBurrierの設定
	//D3D12_RESOURCE_BARRIER barrier_{};
	// 初期値0でFenceを作る
	Microsoft::WRL::ComPtr<ID3D12Fence> fence_ = nullptr;
	HANDLE fenceEvent_ = nullptr;
	// RTVを2つ作るのでディスクリプタを2つ用意
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles_[3]{};

	D3D12_RESOURCE_BARRIER barrier_{};

	// 記録時間(FPS固定用)
	std::chrono::steady_clock::time_point reference_;

	// オフスクリーン用変数
	Microsoft::WRL::ComPtr<ID3D12Resource> offScreenResource_;
	uint32_t srvIndex_;
	std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> srvHandle_;

	D3D12_CLEAR_VALUE clearValue{};


private:

	void InitializeFixFPS();
	void UpdateFixFPS();
public:

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(Microsoft::WRL::ComPtr<ID3D12Device> device, D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptors, bool shaderVisible);

	IDxcBlob* CompileShader(const std::wstring& filePath, const wchar_t* profile);
	Microsoft::WRL::ComPtr<ID3D12Resource> CreateDepthStencilTextureResource(Microsoft::WRL::ComPtr<ID3D12Device> device, int32_t width, int32_t height);
	Microsoft::WRL::ComPtr<ID3D12Resource> CreateTextureResource(const DirectX::TexMetadata& metadata);
	//void UploadTextureData(Microsoft::WRL::ComPtr<ID3D12Resource> texture, const DirectX::ScratchImage& mipImages);
	Microsoft::WRL::ComPtr<ID3D12Resource> UploadTextureData(ID3D12Resource* texture, const DirectX::ScratchImage& mipImages, ID3D12Device* device, ID3D12GraphicsCommandList* commandList);

	void CreateBufferResource(size_t sizeInBytes, Microsoft::WRL::ComPtr<ID3D12Resource>& outResource);

	// オフスクリーン用関数
	Microsoft::WRL::ComPtr<ID3D12Resource> CreateRenderTextureResource(uint32_t width, uint32_t height, DXGI_FORMAT format, D3D12_CLEAR_VALUE color);
	void CreateRTVForOffScreen();
	void CreateSRVForOffScreen(SrvManager* srvManager);
	std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> GetSrvHandle() const { return srvHandle_; }
private:
	void InitializeDXGIDevice();
	// コマンド関連の初期化
	void InitializeCommand();
	// スワップチェーンの生成
	void CreateSwapChain();

	void CreateDepthBuffer();

	void CreateHeap();

	void CreateRenderTargetView();

	void InitializeDepthStencilView();

	void CreateFence();

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
	ID3D12Device* GetDevice() { return device_.Get(); }
	ID3D12GraphicsCommandList* GetCommandList() { return commandList_.Get(); }
	//Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> GetSRVHeap() { return srvHeap_; }
	uint32_t GetDescriptorSizeRTV() { return descriptorSizeRTV_; }
	//uint32_t GetDescriptorSizeSRV() { return descriptorSizeSRV_; }
	uint32_t GetDescriptorSizeDSV() { return descriptorSizeDSV_; }
	// バックバッファの数を取得
	size_t GetBackBufferCount() { return backBuffers_.size(); }
public:
	void TransitionResource(
		ID3D12Resource* resource,
		D3D12_RESOURCE_STATES stateBefore,
		D3D12_RESOURCE_STATES stateAfter);

	void SetRenderTargets(D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle);

	void ClearDepthStencilView();

	void ClearRenderTarget(D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle);

	void SetViewportAndScissorRect();
};