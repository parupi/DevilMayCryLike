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
	void DrawSet(BlendMode blendMode = BlendMode::kNormal, bool toBackBuffer = false);
	// シーンと一緒に描くレイヤー（Background / Game）を描画する。ポストエフェクトがかかる
	void DrawSceneLayers();
	// UIレイヤー（UI / Persistent / Debug）をバックバッファへ直接描画する。
	// ポストエフェクトの後に呼ぶこと（RenderPipeline::Execute）
	void DrawUILayers();
	// HUD（UIレイヤー）の表示を一括で切り替える。死亡演出などで一時的に隠す用。
	// フェードなどの Persistent レイヤーは隠さない。シーン切り替え時に表示へ戻る
	void SetUILayerVisible(bool visible) { isUILayerVisible_ = visible; }
	bool IsUILayerVisible() const { return isUILayerVisible_; }
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

	// first から last までのレイヤーを順番に描画する（last を含む）
	void DrawLayerRange(SpriteLayer first, SpriteLayer last, bool toBackBuffer);

	std::array<std::vector<std::unique_ptr<Sprite>>, static_cast<int32_t>(SpriteLayer::Count)> layers_;
	std::vector<std::unique_ptr<AnimatedSprite>> animatedSprites_;

	bool isUILayerVisible_ = true;
public:
	DirectXManager* GetDxManager() const { return dxManager_; }
};