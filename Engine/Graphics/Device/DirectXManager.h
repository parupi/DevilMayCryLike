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
	// ScratchImage を介さない版。DirectX::Image は画素バッファを指すだけの記述子なので、
	// 呼び出し元が既に持っている画素をそのまま渡せばフルサイズのコピーが1回減る。
	// 数十MBになるアトラステクスチャで効く。
	// images が指す画素は、この関数から戻るまで生きていること。
	Microsoft::WRL::ComPtr<ID3D12Resource> UploadTextureData(
		ID3D12Resource* texture, const DirectX::Image* images, size_t imageCount,
		const DirectX::TexMetadata& metadata);

	/// <summary>
	/// ロード処理を囲むスコープ。
	///
	/// アップロード用のステージングバッファは通常 EndDraw() まで解放されないため、
	/// シーン初期化のように1フレームで大量のテクスチャを読むとその全部が同時に生存し、
	/// ピークメモリが跳ね上がる。このスコープの中では、貯まったステージングが
	/// 一定量を超えるたびに GPU の完了を待って解放するので、ピークが頭打ちになる。
	///
	/// 【重要】内部で使う FlushAndWait() はコマンドリストを Close/Reset するため、
	/// 記録済みのステート（RTV・ディスクリプタヒープ・PSO など）が失われる。
	/// **描画中（BeginDraw〜EndDraw の間）に生存させてはいけない。**
	/// 現在は SceneManager::Update() が scene_->Initialize() を囲むのに使っており、
	/// そこは Draw より前なので安全。
	///
	/// dxManager に nullptr を渡した場合は何もしない（従来どおりの挙動になる）。
	/// </summary>
	class UploadScope {
	public:
		explicit UploadScope(DirectXManager* dxManager);
		~UploadScope();
		UploadScope(const UploadScope&) = delete;
		UploadScope& operator=(const UploadScope&) = delete;
	private:
		DirectXManager* dxManager_ = nullptr;
	};

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

	// 積んだアップロードを確定し、GPU の完了を待ってステージングを解放する
	void FlushUploads();

	// UploadScope の入れ子の深さ。0 のときは容量によるフラッシュを行わない
	// （＝ロード中以外は従来どおり EndDraw まで貯める）
	uint32_t uploadScopeDepth_ = 0;
	// まだ解放していないステージングバッファの合計バイト数
	uint64_t pendingUploadBytes_ = 0;

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
