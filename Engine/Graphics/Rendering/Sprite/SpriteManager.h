#pragma once
#include "Graphics/Device/DirectXManager.h"
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include <array>
#include "Graphics/Rendering/PSO/PSOManager.h"
#include "Sprite.h"
#include "AnimatedSprite.h"

class DirectXManager;
class PSOManager;

// スプライト共通部
class SpriteManager
{
private:
	SpriteManager() = default;
	SpriteManager(const SpriteManager&) = delete;
	SpriteManager& operator=(const SpriteManager&) = delete;

public:
	// シングルトンインスタンスの取得
	static SpriteManager& GetInstance();
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
	// GIF アニメーションスプライトの生成
	// gifFilePath: "Resource/Images/" からの相対パス (例: "UI/animation.gif")
	AnimatedSprite* CreateAnimatedSprite(SpriteLayer layer, const std::string& name, const std::string& gifFilePath);
	// レイヤーの切り替え
	void ChangeLayer(Sprite* sprite, SpriteLayer newLayer);
	// シーンをまたがないスプライトの削除
	void DeleteNonPersistentSprite();
	// 全スプライトの削除
	void DeleteAllSprite();
private:
	// DirectXのポインタ
	DirectXManager* dxManager_ = nullptr;
	PSOManager* psoManager_ = nullptr;

	std::array<std::vector<std::unique_ptr<Sprite>>, static_cast<int32_t>(SpriteLayer::Count)> layers_;
	std::vector<std::unique_ptr<AnimatedSprite>> animatedSprites_;
public:
	DirectXManager* GetDxManager() const { return dxManager_; }
};