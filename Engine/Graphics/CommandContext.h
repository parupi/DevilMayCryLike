#pragma once
#include <wrl/client.h>
#include <d3d12.h>
#include <cstdint>
#include <unordered_map>
class CommandContext
{
public:
	CommandContext() = default;
	~CommandContext();
	// 初期化処理
	bool Initialize(ID3D12Device* device);
	// コマンドのリセット初期化
	void Begin();
	// 使用終了
	void Flush();
	// コマンドの送信と待機
	void FlushAndWait();
	//
	void TransitionResource(ID3D12Resource* resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after);
	// レンダーターゲット設定
	void SetRenderTarget(D3D12_CPU_DESCRIPTOR_HANDLE rtv, D3D12_CPU_DESCRIPTOR_HANDLE dsv);
	// 
	void SetRenderTargets(const D3D12_CPU_DESCRIPTOR_HANDLE* rtvHandles, UINT rtvCount, const D3D12_CPU_DESCRIPTOR_HANDLE* dsvHandle);
	// 
	void ClearRenderTarget(D3D12_CPU_DESCRIPTOR_HANDLE rtv, const FLOAT clearColor[4]);
	// 深度のクリア
	void ClearDepth(D3D12_CPU_DESCRIPTOR_HANDLE dsv);
	// ビューポートとシザーの設定
	void SetViewportAndScissor(const D3D12_VIEWPORT& viewport, const D3D12_RECT& scissorRect);

	// 移動予定
	// フェンス生成
	void CreateFence();

	// コマンドリスト取得
	ID3D12GraphicsCommandList* GetCommandList() { return commandList_.Get(); }
	ID3D12CommandAllocator* GetCommandAllocator() { return commandAllocator_.Get(); }
	ID3D12CommandQueue* GetCommandQueue() { return commandQueue_.Get(); }
private:
	// デバイスのポインタを持っておく
	ID3D12Device* device_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList_ = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Fence> fence_ = nullptr;
	HANDLE fenceEvent_ = nullptr;
	uint64_t fenceValue_ = 0;
	D3D12_RESOURCE_BARRIER barrier_{};

	std::unordered_map<ID3D12Resource*, D3D12_RESOURCE_STATES> resourceStates_;
};

