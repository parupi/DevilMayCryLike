#include "SpriteManager.h"
#include "Graphics/Resource/GifLoader.h"
#include "Graphics/Text/FontManager.h"
#include "Utility/Logger.h"
#include <algorithm>
#include <cassert>

SpriteManager& SpriteManager::GetInstance() {
	static SpriteManager instance;
	return instance;
}

void SpriteManager::Initialize(DirectXManager* directXManager, PSOManager* psoManager) {
	assert(directXManager);
	dxManager_ = directXManager;
	psoManager_ = psoManager;
}

void SpriteManager::DrawSet(BlendMode blendMode, bool toBackBuffer) {
	dxManager_->GetCommandList()->SetPipelineState(psoManager_->GetSpritePSO(blendMode, toBackBuffer));	// PSOを設定
	dxManager_->GetCommandList()->SetGraphicsRootSignature(psoManager_->GetSpriteSignature());
	dxManager_->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void SpriteManager::DrawSceneLayers() {
	DrawLayerRange(SpriteLayer::Background, SpriteLayer::Game, false);
}

void SpriteManager::DrawUILayers() {
	if (isUILayerVisible_) {
		DrawLayerRange(SpriteLayer::UI, SpriteLayer::Debug, true);
	} else {
		// HUDを隠している間もフェードなどの常駐スプライトは描き続ける
		DrawLayerRange(SpriteLayer::Persistent, SpriteLayer::Debug, true);
	}
}

void SpriteManager::DrawLayerRange(SpriteLayer first, SpriteLayer last, bool toBackBuffer) {
	// スプライトと文字を作った順のまま描く。詳細は drawOrder_ のコメントを参照
	for (size_t i = static_cast<size_t>(first); i <= static_cast<size_t>(last); ++i) {
		for (const LayerEntry& entry : drawOrder_[i]) {
			if (entry.sprite) {
				if (!entry.sprite->GetRenderState().isVisible) continue;

				DrawSet(entry.sprite->GetRenderState().blendMode, toBackBuffer);
				entry.sprite->Draw();
			} else if (entry.label) {
				if (!entry.label->GetRenderState().isVisible) continue;

				DrawSet(entry.label->GetRenderState().blendMode, toBackBuffer);
				entry.label->Draw();
			}
		}
	}
}

void SpriteManager::Finalize() {
	DeleteAllSprite();
	GifLoader::ClearCache();
	dxManager_ = nullptr;
	psoManager_ = nullptr;
}

Sprite* SpriteManager::CreateSprite(SpriteLayer layer, const std::string& spriteName, const std::string& textureFilePath) {
	TextureManager::GetInstance().LoadTexture(textureFilePath);
	auto sprite = std::make_unique<Sprite>(spriteName, textureFilePath, layer);

	Sprite* ptr = sprite.get();
	layers_[static_cast<size_t>(layer)].push_back(std::move(sprite));
	drawOrder_[static_cast<size_t>(layer)].push_back(LayerEntry{ ptr, nullptr });

	return ptr;
}

TextLabel* SpriteManager::CreateTextLabel(SpriteLayer layer, const std::string& name, const std::string& fontName) {
	FontManager& fontManager = FontManager::GetInstance();
	Font* font = fontName.empty() ? fontManager.GetDefaultFont() : fontManager.Find(fontName);
	ASSERT_MSG(font != nullptr,
		"[SpriteManager] フォントが読み込まれていません。FontManager::LoadFont を先に呼んでください。");

	auto label = std::make_unique<TextLabel>(name, font, layer);

	TextLabel* ptr = label.get();
	textLayers_[static_cast<size_t>(layer)].push_back(std::move(label));
	drawOrder_[static_cast<size_t>(layer)].push_back(LayerEntry{ nullptr, ptr });

	return ptr;
}

AnimatedSprite* SpriteManager::CreateAnimatedSprite(SpriteLayer layer, const std::string& name, const std::string& gifFilePath) {
	GifInfo info = GifLoader::Load(gifFilePath);
	Sprite* sprite = CreateSprite(layer, name, gifFilePath);
	auto anim = std::make_unique<AnimatedSprite>(sprite, info);
	AnimatedSprite* ptr = anim.get();
	animatedSprites_.push_back(std::move(anim));
	return ptr;
}

void SpriteManager::ChangeLayer(Sprite* sprite, SpriteLayer newLayer) {
	if (!sprite) return;

	auto oldLayer = sprite->layer_;
	if (oldLayer == newLayer) {
		return;
	}

	auto& oldContainer = layers_[static_cast<size_t>(oldLayer)];
	auto& newContainer = layers_[static_cast<size_t>(newLayer)];

	// oldContainerから削除
	for (size_t i = 0; i < oldContainer.size(); ++i) {
		if (oldContainer[i].get() == sprite) {
			// unique_ptrをムーブ
			newContainer.push_back(std::move(oldContainer[i]));

			// swap & pop
			std::swap(oldContainer[i], oldContainer.back());
			oldContainer.pop_back();
			break;
		}
	}

	// 描画順の並びも移す。移動先では末尾＝一番手前になる
	auto& oldOrder = drawOrder_[static_cast<size_t>(oldLayer)];
	oldOrder.erase(
		std::remove_if(oldOrder.begin(), oldOrder.end(),
			[sprite](const LayerEntry& entry) { return entry.sprite == sprite; }),
		oldOrder.end());
	drawOrder_[static_cast<size_t>(newLayer)].push_back(LayerEntry{ sprite, nullptr });

	sprite->layer_ = newLayer;
}

void SpriteManager::DeleteNonPersistentSprite() {
	// シーンが切り替わるのでHUDの一括非表示（死亡演出など）は解除する
	isUILayerVisible_ = true;

	// Persistent レイヤーはシーン切り替えをまたいで生存するためスキップする
	constexpr size_t kPersistentIndex = static_cast<size_t>(SpriteLayer::Persistent);
	for (size_t i = 0; i < layers_.size(); ++i) {
		if (i == kPersistentIndex) continue;
		layers_[i].clear();
		textLayers_[i].clear();
		drawOrder_[i].clear();
	}

	// AnimatedSprite は対応する Sprite が削除されるものだけ除去する
	animatedSprites_.erase(
		std::remove_if(animatedSprites_.begin(), animatedSprites_.end(),
			[](const std::unique_ptr<AnimatedSprite>& anim) {
				return anim->GetSprite()->GetLayer() != SpriteLayer::Persistent;
			}),
		animatedSprites_.end());
}

void SpriteManager::DeleteAllSprite() {
	// AnimatedSprite を先に全削除（Sprite より先に破棄して dangling pointer を避ける）
	animatedSprites_.clear();
	for (auto& layer : layers_) {
		layer.clear();
	}
	for (auto& layer : textLayers_) {
		layer.clear();
	}
	for (auto& order : drawOrder_) {
		order.clear();
	}
}
