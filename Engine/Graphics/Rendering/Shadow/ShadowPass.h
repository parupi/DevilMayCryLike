#pragma once
#include <cstdint>
#include <wrl.h>
#include <d3d12.h>

class DirectXManager;
class PSOManager;
class CascadedShadowMap;
class Object3dManager;

// 影の描画を行う
class ShadowPass
{
public:
	void Initialize(DirectXManager* dxManager, PSOManager* psoManager,
	                CascadedShadowMap* shadowMap, Object3dManager* object3dManager);
	// 描画前処理
	void BeginDraw();
	// 実行
	void Execute();
private:
	DirectXManager* dxManager_ = nullptr;
	PSOManager* psoManager_ = nullptr;
	CascadedShadowMap* shadowMap_ = nullptr;
	Object3dManager* object3dManager_ = nullptr;
};

