#pragma once
#include <wrl/client.h>
#include <d3d12.h>
#include <cstdint>
#include <functional>
class CommandContext
{
public:
	CommandContext() = default;
	~CommandContext();
	// 初期化処理
	bool Initialize(ID3D12Device* device);
	// 次のフレームのコマンドを積み始める。アロケータを2つ交互に使うので、
	// 提出したばかりのフレームを GPU が描いている間に、CPU は次のフレームを積める
	void Begin();
	// 提出する。前に提出したものが GPU で終わるのを待ってから（提出前コールバック → 実行）。
	// 待つのはここだけなので、CPU の更新・コマンド積みと、前のフレームの GPU 処理が重なる
	void Flush();
	// コマンドの送信と待機（ロード中など、GPU の完了まで待ちたいとき）
	void FlushAndWait();
	/// <summary>
	/// 提出の直前（前に提出したものの GPU 処理が終わった後）に呼ぶ処理。
	/// ResourceManager の「CPU 側の写し → GPU のバッファ」の書き写しをここで行う
	/// </summary>
	void SetPreExecuteCallback(std::function<void()> callback) { preExecute_ = std::move(callback); }
	// 前に提出したものの GPU 処理が終わるまで待つ
	void WaitForLastSubmit();
	//
	void TransitionResource(ID3D12Resource* resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after);
	// レンダーターゲット設定
	void SetRenderTarget(D3D12_CPU_DESCRIPTOR_HANDLE rtv, D3D12_CPU_DESCRIPTOR_HANDLE dsv);
	// レンダーターゲット設定
	void SetRenderTarget(D3D12_CPU_DESCRIPTOR_HANDLE rtv);
	// 
	void SetRenderTargets(const D3D12_CPU_DESCRIPTOR_HANDLE* rtvHandles, UINT rtvCount, const D3D12_CPU_DESCRIPTOR_HANDLE* dsvHandle);
	// 
	void ClearRenderTarget(D3D12_CPU_DESCRIPTOR_HANDLE rtv, const FLOAT clearColor[4]);
	// 深度のクリア
	void ClearDepth(D3D12_CPU_DESCRIPTOR_HANDLE dsv);
	// ビューポートとシザーの設定
	void SetViewportAndScissor(const D3D12_VIEWPORT& viewport, const D3D12_RECT& scissorRect);
	// フェンス生成
	void CreateFence();

	ID3D12Fence* GetFence() { return fence_.Get(); }
	uint64_t GetFenceValue() const { return fenceValue_; }

	// コマンドリスト取得
	ID3D12GraphicsCommandList* GetCommandList() { return commandList_.Get(); }
	ID3D12CommandAllocator* GetCommandAllocator() { return commandAllocators_[allocatorIndex_].Get(); }
	ID3D12CommandQueue* GetCommandQueue() { return commandQueue_.Get(); }
private:
	// 閉じたコマンドリストを、前の提出の完了を待ってから実行する（Flush / FlushAndWait 共通）
	void ExecuteClosedList();
	void WaitForFenceValue(uint64_t value);

	// GPU に同時に積んでおけるフレーム数（＝アロケータの数）
	static constexpr uint32_t kAllocatorCount = 2;

	// デバイスのポインタを持っておく
	ID3D12Device* device_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocators_[kAllocatorCount];
	// それぞれのアロケータで積んだコマンドを最後に提出したときのフェンス値（リセットしてよいかの判断）
	uint64_t allocatorFenceValues_[kAllocatorCount]{};
	uint32_t allocatorIndex_ = 0;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList_ = nullptr;
	std::function<void()> preExecute_;

	Microsoft::WRL::ComPtr<ID3D12Fence> fence_ = nullptr;
	HANDLE fenceEvent_ = nullptr;
	uint64_t fenceValue_ = 0;
};

