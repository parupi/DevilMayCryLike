#include "SpeedLineEffect.h"
#include "OffScreenManager.h"
#include <cmath>
#include <Utility/TimeManager.h>

SpeedLineEffect::SpeedLineEffect(const std::string& name)
{
	name_ = name;
	dxManager_ = OffScreenManager::GetInstance().GetDXManager();
	psoManager_ = OffScreenManager::GetInstance().GetPSOManager();

	effectData_.aspect = static_cast<float>(WindowManager::kGameWidth) / static_cast<float>(WindowManager::kGameHeight);

	CreateEffectResource();
}

SpeedLineEffect::~SpeedLineEffect()
{
	// 借りてるポインタを破棄
	dxManager_ = nullptr;
	psoManager_ = nullptr;
	// 生成したリソースの削除
	effectHandle_ = 0;
}

void SpeedLineEffect::Update()
{
	// ちらつきはVFXと同じ時間で動かす。float の桁が落ちないよう1時間で畳む
	effectData_.time = std::fmod(effectData_.time + TimeManager::GetVFXDelta(), 3600.0f);

	// CPU → GPU に全フィールドをコピー（パディング含む）
	*effectDataPtr_ = effectData_;
}

void SpeedLineEffect::Draw()
{
	dxManager_->GetCommandList()->SetPipelineState(psoManager_->GetOffScreenPSO(OffScreenEffectType::kSpeedLine));
	dxManager_->GetCommandList()->SetGraphicsRootSignature(psoManager_->GetOffScreenSignature());
	dxManager_->GetCommandList()->SetGraphicsRootDescriptorTable(0, inputSrv_);

	dxManager_->GetCommandList()->SetGraphicsRootConstantBufferView(1, dxManager_->GetResourceManager()->GetGPUVirtualAddress(effectHandle_));

	dxManager_->GetCommandList()->DrawInstanced(3, 1, 0, 0);
}

void SpeedLineEffect::CreateEffectResource()
{
	auto* resourceManager = dxManager_->GetResourceManager();
	effectHandle_ = resourceManager->CreateUploadBuffer(sizeof(SpeedLineData), L"SpeedLineEffect");
	effectDataPtr_ = reinterpret_cast<SpeedLineData*>(resourceManager->Map(effectHandle_));
	*effectDataPtr_ = effectData_;
}
