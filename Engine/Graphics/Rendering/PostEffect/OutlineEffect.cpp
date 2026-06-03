#include "OutlineEffect.h"
#include "OffScreenManager.h"
#include <imgui.h>

OutlineEffect::OutlineEffect()
{
	dxManager_ = OffScreenManager::GetInstance()->GetDXManager();
	psoManager_ = OffScreenManager::GetInstance()->GetPSOManager();

	CreateEffectResource();
}

OutlineEffect::~OutlineEffect()
{
	// 借りてるポインタを破棄
	dxManager_ = nullptr;
	psoManager_ = nullptr;
	// 生成したリソースの削除
	effectData_ = nullptr;
	effectHandle_ = 0;
}

void OutlineEffect::Update()
{
#ifdef _DEBUG
	ImGui::Begin("OutlineEffect");
	ImGui::Checkbox("isActive", &isActive_);

	ImGui::End();
#endif // _DEBUG
}

void OutlineEffect::Draw()
{
	dxManager_->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	dxManager_->GetCommandList()->SetPipelineState(psoManager_->GetOffScreenPSO(OffScreenEffectType::kOutLine));
	dxManager_->GetCommandList()->SetGraphicsRootSignature(psoManager_->GetOffScreenSignature());
	dxManager_->GetCommandList()->SetGraphicsRootDescriptorTable(0, inputSrv_);

	dxManager_->GetCommandList()->SetGraphicsRootConstantBufferView(1, dxManager_->GetResourceManager()->GetGPUVirtualAddress(effectHandle_));

	dxManager_->GetCommandList()->DrawInstanced(3, 1, 0, 0);
}

void OutlineEffect::CreateEffectResource()
{
	auto* resourceManager = dxManager_->GetResourceManager();
	// ヴィネット用のリソースを作る
	effectHandle_ = resourceManager->CreateUploadBuffer(sizeof(OutlineEffectData), L"GaussianEffect");
	// 書き込むためのアドレスを取得
	effectData_ = reinterpret_cast<OutlineEffectData*>(resourceManager->Map(effectHandle_));
}
