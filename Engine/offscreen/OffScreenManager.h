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
	// エフェクトを探す
	BaseOffScreen* FindEffect(const std::string& name);

	void BeginDrawToPingPong();

	void EndDrawToPingPong();

	// アクセッサ
	DirectXManager* GetDXManager() { return dxManager_; }
	PSOManager* GetPSOManager() { return psoManager_; }
private:
	Microsoft::WRL::ComPtr<ID3D12Resource> CreateOffScreenRenderTarget();
	uint32_t CreateRTVForResource(Microsoft::WRL::ComPtr<ID3D12Resource>);
	uint32_t CreateSRVForResource(Microsoft::WRL::ComPtr<ID3D12Resource>);

private:
	DirectXManager* dxManager_ = nullptr;
	PSOManager* psoManager_ = nullptr;
	SrvManager* srvManager_ = nullptr;

	// Ping/Pong buffers
	Microsoft::WRL::ComPtr<ID3D12Resource> pingPongBuffers_[2];
	uint32_t rtvIndices_[2] = { UINT32_MAX, UINT32_MAX }; // index in rtvManager
	uint32_t srvIndices_[2] = { UINT32_MAX, UINT32_MAX }; // index in srvManager

	// GPU descriptor handles (for binding SRV to root table)
	D3D12_GPU_DESCRIPTOR_HANDLE srvHandles_[2]{};
	// keep CPU handles for DirectXManager::SetRenderTarget if needed
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles_[2]{};

	// ping/pong indicators
	int ping_ = 0;
	int pong_ = 1;

	// viewport / scissor for offscreen
	D3D12_VIEWPORT viewport_{};
	D3D12_RECT scissorRect_{};
	D3D12_CLEAR_VALUE clearValue_{};

	// effects
	std::vector<std::unique_ptr<BaseOffScreen>> effects_;
};

