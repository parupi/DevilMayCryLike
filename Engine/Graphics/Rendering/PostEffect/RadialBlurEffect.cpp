#include "RadialBlurEffect.h"
#include "OffScreenManager.h"

RadialBlurEffect::RadialBlurEffect(const std::string& name)
{
	name_ = name;
	dxManager_ = OffScreenManager::GetInstance().GetDXManager();
	psoManager_ = OffScreenManager::GetInstance().GetPSOManager();

	CreateEffectResource();
}

RadialBlurEffect::~RadialBlurEffect()
{
	// 借りてるポインタを破棄
	dxManager_ = nullptr;
	psoManager_ = nullptr;
	// 生成したリソースの削除
	effectHandle_ = 0;
}

void RadialBlurEffect::Update()
{
	// CPU → GPU に全フィールドをコピー（パディング含む）
	*effectDataPtr_ = effectData_;
}

void RadialBlurEffect::Draw()
{
	dxManager_->GetCommandList()->SetPipelineState(psoManager_->GetOffScreenPSO(OffScreenEffectType::kRadialBlur));
	dxManager_->GetCommandList()->SetGraphicsRootSignature(psoManager_->GetOffScreenSignature());
	dxManager_->GetCommandList()->SetGraphicsRootDescriptorTable(0, inputSrv_);

	dxManager_->GetCommandList()->SetGraphicsRootConstantBufferView(1, dxManager_->GetResourceManager()->GetGPUVirtualAddress(effectHandle_));

	dxManager_->GetCommandList()->DrawInstanced(3, 1, 0, 0);
}

void RadialBlurEffect::CreateEffectResource()
{
	auto* resourceManager = dxManager_->GetResourceManager();
	effectHandle_ = resourceManager->CreateUploadBuffer(sizeof(RadialBlurData), L"RadialBlurEffect");
	effectDataPtr_ = reinterpret_cast<RadialBlurData*>(resourceManager->Map(effectHandle_));
	*effectDataPtr_ = effectData_;
}
