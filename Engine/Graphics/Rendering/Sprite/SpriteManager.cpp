#include "SpriteManager.h"
#include "Graphics/Resource/GifLoader.h"
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

void SpriteManager::DrawSet(BlendMode blendMode) {
	dxManager_->GetCommandList()->SetPipelineState(psoManager_->GetSpritePSO(blendMode));			// PSOを設定
	dxManager_->GetCommandList()->SetGraphicsRootSignature(psoManager_->GetSpriteSignature());
	dxManager_->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void SpriteManager::DrawAllSprite() {
	for (auto& layer : layers_) {
		for (auto& sprite : layer) {
			if (!sprite->GetRenderState().isVisible) continue;

			DrawSet(sprite->GetRenderState().blendMode);
			sprite->Draw();
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
	auto sprite = std::make_unique<Sprite>(spriteName, textureFilePath, layer);

	Sprite* ptr = sprite.get();
	layers_[static_cast<size_t>(layer)].push_back(std::move(sprite));

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

	sprite->layer_ = newLayer;
}

void SpriteManager::DeleteNonPersistentSprite() {
	// Persistent レイヤーはシーン切り替えをまたいで生存するためスキップする
	constexpr size_t kPersistentIndex = static_cast<size_t>(SpriteLayer::Persistent);
	for (size_t i = 0; i < layers_.size(); ++i) {
		if (i == kPersistentIndex) continue;
		layers_[i].clear();
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
}
