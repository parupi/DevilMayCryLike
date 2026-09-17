#include "HitFlashEffect.h"
#include "OffScreenManager.h"

HitFlashEffect::HitFlashEffect(const std::string& name)
{
	name_ = name;
	dxManager_ = OffScreenManager::GetInstance().GetDXManager();
	psoManager_ = OffScreenManager::GetInstance().GetPSOManager();

	CreateEffectResource();
}

HitFlashEffect::~HitFlashEffect()
{
	// 借りてるポインタを破棄
	dxManager_ = nullptr;
	psoManager_ = nullptr;
	// 生成したリソースの削除
	effectHandle_ = 0;
}

void HitFlashEffect::Update()
{
	// CPU → GPU に全フィールドをコピー
	*effectDataPtr_ = effectData_;
}

void HitFlashEffect::Draw()
{
	dxManager_->GetCommandList()->SetPipelineState(psoManager_->GetOffScreenPSO(OffScreenEffectType::kHitFlash));
	dxManager_->GetCommandList()->SetGraphicsRootSignature(psoManager_->GetOffScreenSignature());
	dxManager_->GetCommandList()->SetGraphicsRootDescriptorTable(0, inputSrv_);

	dxManager_->GetCommandList()->SetGraphicsRootConstantBufferView(1, dxManager_->GetResourceManager()->GetGPUVirtualAddress(effectHandle_));

	dxManager_->GetCommandList()->DrawInstanced(3, 1, 0, 0);
}

void HitFlashEffect::CreateEffectResource()
{
	auto* resourceManager = dxManager_->GetResourceManager();
	effectHandle_ = resourceManager->CreateUploadBuffer(sizeof(HitFlashData), L"HitFlashEffect");
	effectDataPtr_ = reinterpret_cast<HitFlashData*>(resourceManager->Map(effectHandle_));
	*effectDataPtr_ = effectData_;
}
