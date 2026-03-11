#include "SpriteManager.h"
#include <cassert>

SpriteManager* SpriteManager::instance = nullptr;
std::once_flag SpriteManager::initInstanceFlag;

SpriteManager* SpriteManager::GetInstance()
{
	std::call_once(initInstanceFlag, []() {
		instance = new SpriteManager();
		});
	return instance;
}

void SpriteManager::Initialize(DirectXManager* directXManager, PSOManager* psoManager) {
	assert(directXManager);
	dxManager_ = directXManager;
	psoManager_ = psoManager;
}

void SpriteManager::DrawSet(BlendMode blendMode)
{
	dxManager_->GetCommandList()->SetPipelineState(psoManager_->GetSpritePSO(blendMode));			// PSOを設定
	dxManager_->GetCommandList()->SetGraphicsRootSignature(psoManager_->GetSpriteSignature());
	dxManager_->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void SpriteManager::DrawAllSprite()
{
	for (auto& layer : layers_) {
		for (auto& sprite : layer) {
			if (!sprite->GetRenderState().isVisible) continue;

			DrawSet(sprite->GetRenderState().blendMode);
			sprite->Draw();
		}
	}
}

void SpriteManager::Finalize()
{
	dxManager_ = nullptr;
	psoManager_ = nullptr;

	delete instance;
	instance = nullptr;
}

Sprite* SpriteManager::CreateSprite(SpriteLayer layer, const std::string& spriteName, const std::string& textureFilePath)
{
	auto sprite = std::make_unique<Sprite>(spriteName, textureFilePath, layer);

	Sprite* ptr = sprite.get();
	layers_[static_cast<size_t>(layer)].push_back(std::move(sprite));

	return ptr;
}

void SpriteManager::ChangeLayer(Sprite* sprite, SpriteLayer newLayer)
{
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

void SpriteManager::DeleteAllSprite()
{
	for (auto& layer : layers_) {
		for (auto& sprite : layer) {
			sprite.release();
		}
		layer.clear();
	}
}
