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

void SpriteManager::Finalize()
{
	dxManager_ = nullptr;
	psoManager_ = nullptr;

	delete instance;
	instance = nullptr;
}
