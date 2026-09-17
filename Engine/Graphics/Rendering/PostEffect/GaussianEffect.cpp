#include "GaussianEffect.h"
#include "OffScreenManager.h"

GaussianEffect::GaussianEffect()
{
	dxManager_ = OffScreenManager::GetInstance().GetDXManager();
	psoManager_ = OffScreenManager::GetInstance().GetPSOManager();

	CreateEffectResource();
}

GaussianEffect::~GaussianEffect()
{
	// 借りてるポインタを破棄
	dxManager_ = nullptr;
	psoManager_ = nullptr;
	// 生成したリソースの削除
	effectData_ = nullptr;
	effectHandle_ = 0;
}

void GaussianEffect::Update()
{
}

void GaussianEffect::Draw()
{
	dxManager_->GetCommandList()->SetPipelineState(psoManager_->GetOffScreenPSO(OffScreenEffectType::kGauss));
	dxManager_->GetCommandList()->SetGraphicsRootSignature(psoManager_->GetOffScreenSignature());
	dxManager_->GetCommandList()->SetGraphicsRootDescriptorTable(0, inputSrv_);

	dxManager_->GetCommandList()->SetGraphicsRootConstantBufferView(1, dxManager_->GetResourceManager()->GetGPUVirtualAddress(effectHandle_));

	dxManager_->GetCommandList()->DrawInstanced(3, 1, 0, 0);
}

void GaussianEffect::CreateEffectResource()
{
	auto* resourceManager = dxManager_->GetResourceManager();
	// ガウス用のリソースを作る
	effectHandle_ = resourceManager->CreateUploadBuffer(sizeof(GaussianEffectData), L"GaussianEffect");
	// 書き込むためのアドレスを取得
	effectData_ = reinterpret_cast<GaussianEffectData*>(resourceManager->Map(effectHandle_));
	// 初期値を設定
	effectData_->sigma = 10.0f;
	effectData_->blurStrength = 1.0f;
	effectData_->alphaMode = 1.0f;
	effectData_->uvClampMin = { -1.0f, -1.0f };
	effectData_->uvClampMax = { 1.0f, 1.0f };
}
