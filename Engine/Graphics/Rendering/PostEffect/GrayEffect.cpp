#include "GrayEffect.h"
#include "Graphics/Rendering/PSO/PSOManager.h"
#include "OffScreenManager.h"
#include <algorithm>

GrayEffect::GrayEffect(const std::string& name) : BaseOffScreen()
{
	name_ = name;
	dxManager_ = OffScreenManager::GetInstance().GetDXManager();
	psoManager_ = OffScreenManager::GetInstance().GetPSOManager();

	CreateEffectResource();
}

GrayEffect::~GrayEffect()
{
	// 借りてるポインタを破棄
	dxManager_ = nullptr;
	psoManager_ = nullptr;

	// 生成したリソースの削除
	effectData_ = nullptr;
	effectHandle_ = 0;
}

void GrayEffect::Update()
{
}

void GrayEffect::Draw()
{
	dxManager_->GetCommandList()->SetPipelineState(psoManager_->GetOffScreenPSO(OffScreenEffectType::kGray));
	dxManager_->GetCommandList()->SetGraphicsRootSignature(psoManager_->GetOffScreenSignature());
	dxManager_->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	dxManager_->GetCommandList()->SetGraphicsRootDescriptorTable(0, inputSrv_);

	dxManager_->GetCommandList()->SetGraphicsRootConstantBufferView(1, dxManager_->GetResourceManager()->GetGPUVirtualAddress(effectHandle_));

	dxManager_->GetCommandList()->DrawInstanced(3, 1, 0, 0);
}

void GrayEffect::CreateEffectResource()
{
	auto* resourceManager = dxManager_->GetResourceManager();
	// グレー用のリソースを作る
	effectHandle_ = resourceManager->CreateUploadBuffer(sizeof(GrayEffectData), L"GrayEffect");
	// 書き込むためのアドレスを取得
	effectData_ = reinterpret_cast<GrayEffectData*>(resourceManager->Map(effectHandle_));
	// 初期値を設定
	effectData_->intensity = 1.0f;
}
