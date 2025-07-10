#pragma once
#include "base/DirectXManager.h"
#include <mutex>
#include "base/PSOManager.h"
class OffScreen
{
public:
	// 初期化
	void Initialize(DirectXManager* dxManager, PSOManager* psoManager);
	// 終了
	void Finalize();
	// 更新処理
	void Update();
	// 描画
	void Draw();

private:
	DirectXManager* dxManager_ = nullptr;
	PSOManager* psoManager_ = nullptr;

	OffScreenEffectType effectType_ = OffScreenEffectType::kNone;
};

