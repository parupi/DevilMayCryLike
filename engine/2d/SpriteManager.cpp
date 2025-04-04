#include "SpriteManager.h"

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
	dxManager_->GetCommandList()->SetPipelineState(psoManager_->GetSpritePSO(blendMode).Get());			// PSOを設定
	dxManager_->GetCommandList()->SetGraphicsRootSignature(psoManager_->GetSpriteSignature().Get());
	dxManager_->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void SpriteManager::Finalize()
{
	delete instance;
	instance = nullptr;
}
