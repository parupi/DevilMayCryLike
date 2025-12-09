#pragma once
#include <wrl.h>
#include <d3d12.h>
#include <cstdint>

class DirectXManager;
class PSOManager;

class CompositePath
{
public:
	CompositePath() = default;
	~CompositePath() = default;

	// 初期化処理
	void Initialize(DirectXManager* dxManager, PSOManager* psoManager);
	// 合成
	void Composite(uint32_t forwardSrvIndex, uint32_t forwardDsvIndex, uint32_t DeferredSrvIndex, uint32_t deferredDsvIndex);
	// 生成したSrvの取得
	uint32_t GetSrvIndex() const { return srvIndex_; }
private:
	// リソースを生成
	void CreateResource();

	DirectXManager* dxManager_;
	PSOManager* psoManager_;

	Microsoft::WRL::ComPtr<ID3D12Resource> rtvResource_;
	uint32_t rtvIndex_ = 0;

	Microsoft::WRL::ComPtr<ID3D12Resource> srvResource_;
	uint32_t srvIndex_ = 0;
};

