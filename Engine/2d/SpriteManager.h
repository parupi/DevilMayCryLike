#pragma once
#include "Graphics/Device/DirectXManager.h"
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include <array>
#include "Graphics/Rendering/PSO/PSOManager.h"
#include "Sprite.h"

class DirectXManager;
class PSOManager;

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
	// 全スプライトを描画
	void DrawAllSprite();
	// 終了
	void Finalize();
	// スプライトの生成
	Sprite* CreateSprite(SpriteLayer layer, const std::string& spriteName, const std::string& textureFilePath);
	// レイヤーの切り替え
	void ChangeLayer(Sprite* sprite, SpriteLayer newLayer);
	// スプライトの削除
	void DeleteAllSprite();
private:
	// DirectXのポインタ
	DirectXManager* dxManager_ = nullptr;
	PSOManager* psoManager_ = nullptr;

	//std::vector<std::unique_ptr<Sprite>> sprites_;

	std::array<std::vector<std::unique_ptr<Sprite>>, static_cast<int32_t>(SpriteLayer::Count)> layers_;
public:
	DirectXManager* GetDxManager() const { return dxManager_; }
};