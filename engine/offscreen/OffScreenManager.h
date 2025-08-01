#pragma once
#include <mutex>
#include <base/PSOManager.h>
#include <base/DirectXManager.h>
#include "BaseOffScreen.h"
class OffScreenManager
{
private:
	static OffScreenManager* instance;
	static std::once_flag initInstanceFlag;

	OffScreenManager() = default;
	~OffScreenManager() = default;
	OffScreenManager(const OffScreenManager&) = default;
	OffScreenManager& operator=(const OffScreenManager&) = default;
public:
	// シングルトンインスタンスの取得
	static OffScreenManager* GetInstance();
	// 初期化処理
	void Initialize(DirectXManager* dxManager, PSOManager* psoManager, SrvManager* srvManager);
	// 終了
	void Finalize();
	// 更新処理
	void Update();
	// 描画
	void DrawPostEffect();
	// オフスクリーンの追加
	void AddEffect(std::unique_ptr<BaseOffScreen> effect);

	void BeginDrawToPingPong();

	void EndDrawToPingPong();

	// アクセッサ
	DirectXManager* GetDXManager() { return dxManager_; }
	PSOManager* GetPSOManager() { return psoManager_; }
private:
	Microsoft::WRL::ComPtr<ID3D12Resource> CreateOffScreenRenderTarget();
	D3D12_CPU_DESCRIPTOR_HANDLE CreateRTV(Microsoft::WRL::ComPtr<ID3D12Resource>);
	D3D12_GPU_DESCRIPTOR_HANDLE CreateSRV(Microsoft::WRL::ComPtr<ID3D12Resource>);

private:
	DirectXManager* dxManager_ = nullptr;
	PSOManager* psoManager_ = nullptr;
	SrvManager* srvManager_ = nullptr;

	std::vector<std::unique_ptr<BaseOffScreen>> effects_;

	// オフスクリーン描画用Ping-Pongバッファ
	//Microsoft::WRL::ComPtr<ID3D12Resource> pingPongBufferA_;
	//Microsoft::WRL::ComPtr<ID3D12Resource> pingPongBufferB_;
	Microsoft::WRL::ComPtr<ID3D12Resource> pingPongBuffers_[2];

	int ping_ = 0;
	int pong_ = 1;

	// SRV, RTV ハンドルなども用意する必要あり
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles_[2]; // A, B
	D3D12_GPU_DESCRIPTOR_HANDLE srvHandles_[2];

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvHeap_;

	uint32_t descriptorSizeRTV_;

	D3D12_VIEWPORT viewport_;
	D3D12_RECT scissorRect_;
	D3D12_CLEAR_VALUE clearValue_;

	D3D12_RESOURCE_STATES pingPongStates_[2] = {
		D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_GENERIC_READ
	};
};

