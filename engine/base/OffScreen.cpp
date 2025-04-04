#include "OffScreen.h"

void OffScreen::Initialize(DirectXManager* dxManager, PSOManager* psoManager)
{
	dxManager_ = dxManager;
	psoManager_ = psoManager;

}

void OffScreen::Finalize()
{
}

void OffScreen::Draw(OffScreenEffectType effectType)
{
	dxManager_->GetCommandList()->SetPipelineState(psoManager_->GetOffScreenPSO(effectType).Get());
	dxManager_->GetCommandList()->SetGraphicsRootSignature(psoManager_->GetOffScreenSignature().Get());
	dxManager_->GetCommandList()->SetGraphicsRootDescriptorTable(0, dxManager_->GetSrvHandle().second);
	dxManager_->GetCommandList()->DrawInstanced(3, 1, 0, 1);
}