#pragma once
#include <mutex>
#include <base/PSOManager.h>
#include <base/DirectXManager.h>
#include "BaseOffScreen.h"
#include "PostEffectPath.h"
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
	void Initialize(DirectXManager* dxManager, PSOManager* psoManager);
	// 終了
	void Finalize();
	// 更新処理
	void Update();
	// 描画
	void DrawPostEffect();
	// すべてのポストエフェクトを順番に適用
	void ExecutePostEffects(); 
	// オフスクリーンの追加
	void AddEffect(std::unique_ptr<BaseOffScreen> effect);
	// エフェクトを探す
	BaseOffScreen* FindEffect(const std::string& name);
	// 全エフェクトを取得
	std::vector<BaseOffScreen*> GetEffects();
	// 描画前処理
	void BeginDrawToPingPong();
	// 描画終了処理
	void EndDrawToPingPong();

	// アクセッサ
	DirectXManager* GetDXManager() { return dxManager_; }
	PSOManager* GetPSOManager() { return psoManager_; }

	D3D12_GPU_DESCRIPTOR_HANDLE GetFinalPostEffectSrv() const { return finalPostEffectSrv_; }
	bool HasPostEffectResult() const { return didHavePostEffectResult_; }

	ID3D12Resource* GetPingBuffer() const { return pingPongBuffers_[ping_].Get(); }
	D3D12_CPU_DESCRIPTOR_HANDLE GetRtvHandlePing() const { return rtvHandles_[ping_]; }
	D3D12_GPU_DESCRIPTOR_HANDLE GetSrvHandlePing() const { return srvHandles_[ping_]; }
	D3D12_GPU_DESCRIPTOR_HANDLE GetSrvHandlePong() const { return srvHandles_[pong_]; }
	UINT& GetPingIndexRef() { return ping_; }
	UINT& GetPongIndexRef() { return pong_; }
	const float* GetClearColor() const { return clearValue_.Color; }
	const D3D12_VIEWPORT& GetViewport() const { return viewport_; }
	const D3D12_RECT& GetScissorRect() const { return scissorRect_; }
private:
	Microsoft::WRL::ComPtr<ID3D12Resource> CreateOffScreenRenderTarget();
	uint32_t CreateRTVForResource(Microsoft::WRL::ComPtr<ID3D12Resource>);
	uint32_t CreateSRVForResource(Microsoft::WRL::ComPtr<ID3D12Resource>);

private:
	DirectXManager* dxManager_ = nullptr;
	PSOManager* psoManager_ = nullptr;
	//SrvManager* srvManager_ = nullptr;

	// Ping/Pong buffers
	static constexpr UINT kPingPongCount = 2;
	Microsoft::WRL::ComPtr<ID3D12Resource> pingPongBuffers_[kPingPongCount];
	uint32_t rtvIndices_[2] = { UINT32_MAX, UINT32_MAX }; // index in rtvManager
	uint32_t srvIndices_[2] = { UINT32_MAX, UINT32_MAX }; // index in srvManager

	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles_[kPingPongCount]{};
	D3D12_GPU_DESCRIPTOR_HANDLE srvHandles_[kPingPongCount]{};
	UINT ping_ = 0;
	UINT pong_ = 1;

	D3D12_GPU_DESCRIPTOR_HANDLE finalPostEffectSrv_;
	bool didHavePostEffectResult_ = false;

	
	D3D12_VIEWPORT viewport_{};
	D3D12_RECT scissorRect_{};
	D3D12_CLEAR_VALUE clearValue_{};

	// effects
	std::vector<std::unique_ptr<BaseOffScreen>> effects_;
	std::vector<PostEffectPath*> paths_;
};

