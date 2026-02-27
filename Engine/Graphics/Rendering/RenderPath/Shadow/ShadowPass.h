#pragma once
#include <cstdint>
#include <wrl.h>
#include <d3d12.h>

class DirectXManager;
class PSOManager;
class CascadedShadowMap;

// 影の描画を行う
class ShadowPass
{
public:
	// 初期化
	void Initialize(DirectXManager* dxManager, PSOManager* psoManager, CascadedShadowMap* shadowMap);
	// 描画前処理
	void BeginDraw();
	// 実行
	void Execute();
	// 描画後処理
	void EndDraw();
private:
	void CreateResource();

private:
	DirectXManager* dxManager_ = nullptr;
	PSOManager* psoManager_ = nullptr;
	CascadedShadowMap* shadowMap_ = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> depthBuffer_ = nullptr;
	uint32_t dsvIndex_ = 0;
	uint32_t srvIndex_ = 0;
};

