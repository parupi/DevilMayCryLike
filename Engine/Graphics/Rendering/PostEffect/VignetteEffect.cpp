#include "VignetteEffect.h"
#include "OffScreenManager.h"

VignetteEffect::VignetteEffect(const std::string& name)
{
	name_ = name;
	dxManager_ = OffScreenManager::GetInstance().GetDXManager();
	psoManager_ = OffScreenManager::GetInstance().GetPSOManager();

	CreateEffectResource();
}

VignetteEffect::~VignetteEffect()
{
	// 借りてるポインタを破棄
	dxManager_ = nullptr;
	psoManager_ = nullptr;
	// 生成したリソースの削除
	effectHandle_ = 0;
}

void VignetteEffect::Update()
{

	// CPU → GPU に全フィールドをコピー（パディング含む）
	*effectDataPtr_ = effectData_;
}

void VignetteEffect::Draw()
{
	dxManager_->GetCommandList()->SetPipelineState(psoManager_->GetOffScreenPSO(OffScreenEffectType::kVignette));
	dxManager_->GetCommandList()->SetGraphicsRootSignature(psoManager_->GetOffScreenSignature());
	dxManager_->GetCommandList()->SetGraphicsRootDescriptorTable(0, inputSrv_);

	dxManager_->GetCommandList()->SetGraphicsRootConstantBufferView(1, dxManager_->GetResourceManager()->GetGPUVirtualAddress(effectHandle_));

	dxManager_->GetCommandList()->DrawInstanced(3, 1, 0, 0);
}

void VignetteEffect::CreateEffectResource()
{
	auto* resourceManager = dxManager_->GetResourceManager();
	// ヴィネット用のリソースを作る
	effectHandle_ = resourceManager->CreateUploadBuffer(sizeof(VignetteEffectData), L"VignetteEffect");
	// 書き込むためのアドレスを取得
	effectDataPtr_ = reinterpret_cast<VignetteEffectData*>(resourceManager->Map(effectHandle_));
	// 初期値を設定
	effectDataPtr_->radius = 0.5f;
	effectDataPtr_->intensity = 0.5f;
	effectDataPtr_->softness = -0.1f;
}
