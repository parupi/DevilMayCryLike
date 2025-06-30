#include "SmoothEffect.h"
#include "OffScreenManager.h"
#include <imgui.h>

SmoothEffect::SmoothEffect() 
{
	dxManager_ = OffScreenManager::GetInstance()->GetDXManager();
	psoManager_ = OffScreenManager::GetInstance()->GetPSOManager();

	CreateEffectResource();
}

void SmoothEffect::Update()
{
#ifdef _DEBUG
	ImGui::Begin("SmoothEffect");
	ImGui::Checkbox("isActive", &isActive_);
	ImGui::DragFloat("blurStrength", &effectData_->blurStrength, 0.01f);
	ImGui::DragInt("iterations", &effectData_->iterations);
	ImGui::End();
#endif // _DEBUG
}

void SmoothEffect::Draw()
{
	dxManager_->GetCommandList()->SetPipelineState(psoManager_->GetOffScreenPSO(OffScreenEffectType::kSmooth));
	dxManager_->GetCommandList()->SetGraphicsRootSignature(psoManager_->GetOffScreenSignature());
	dxManager_->GetCommandList()->SetGraphicsRootDescriptorTable(0, dxManager_->GetSrvHandle().second);

	dxManager_->GetCommandList()->SetGraphicsRootConstantBufferView(1, effectResource_->GetGPUVirtualAddress());

	dxManager_->GetCommandList()->DrawInstanced(3, 1, 0, 1);
}

void SmoothEffect::CreateEffectResource()
{
	// ヴィネット用のリソースを作る
	effectResource_ = dxManager_->CreateBufferResource(sizeof(SmoothEffectData));
	// 書き込むためのアドレスを取得
	effectResource_->Map(0, nullptr, reinterpret_cast<void**>(&effectData_));
	// 初期値を設定
	effectData_->blurStrength = 1.0f;
	effectData_->iterations = 1;
}
