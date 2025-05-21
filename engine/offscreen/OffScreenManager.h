#pragma once
#include <mutex>
#include <PSOManager.h>
#include <DirectXManager.h>
#include "BaseOffScreen.h"
class OffScreenManager
{
private:
	static OffScreenManager* instance;
	static std::once_flag initInstanceFlag;

	OffScreenManager() = default;
	~OffScreenManager() = default;
	OffScreenManager(OffScreenManager&) = default;
	OffScreenManager& operator=(OffScreenManager&) = default;
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
	void Draw(/*OffScreenEffectType effectType = OffScreenEffectType::kNone*/);
	// オフスクリーンの追加
	void AddEfect(std::unique_ptr<BaseOffScreen> effect);

	// アクセッサ
	DirectXManager* GetDXManager() { return dxManager_; }
	PSOManager* GetPSOManager() { return psoManager_; }
private:
	DirectXManager* dxManager_ = nullptr;
	PSOManager* psoManager_ = nullptr;

	std::vector<std::unique_ptr<BaseOffScreen>> effects_;
};

