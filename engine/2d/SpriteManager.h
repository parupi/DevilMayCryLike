#pragma once
#include "base/DirectXManager.h"
#include <memory>
#include <mutex>
#include "base/PSOManager.h"
// スプライト共通部
class SpriteManager
{
public:
	static SpriteManager* instance;
	static std::once_flag initInstanceFlag;

	SpriteManager() = default;
	~SpriteManager() = default;
	SpriteManager(SpriteManager&) = default;
	SpriteManager& operator=(SpriteManager&) = default;
public:
	// シングルトンインスタンスの取得
	static SpriteManager* GetInstance();
	// 初期化
	void Initialize(DirectXManager* directXManager, PSOManager* psoManager);
	// 描画前処理
	void DrawSet(BlendMode blendMode = BlendMode::kNormal);
	// 終了
	void Finalize();
private:
	// DirectXのポインタ
	DirectXManager* dxManager_ = nullptr;
	PSOManager* psoManager_ = nullptr;

public:
	DirectXManager* GetDxManager() const { return dxManager_; }
};