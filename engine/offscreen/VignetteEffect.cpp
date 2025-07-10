#include "VignetteEffect.h"
#include "OffScreenManager.h"
#include <imgui/imgui.h>

VignetteEffect::VignetteEffect()
{
	dxManager_ = OffScreenManager::GetInstance()->GetDXManager();
	psoManager_ = OffScreenManager::GetInstance()->GetPSOManager();

	CreateEffectResource();
}

void VignetteEffect::Update()
{
#ifdef _DEBUG
	ImGui::Begin("VignetteEffect");
	ImGui::Checkbox("isActive", &isActive_);
	ImGui::DragFloat("radius", &effectData_->radius, 0.01f);
	ImGui::DragFloat("intensity", &effectData_->intensity, 0.01f);
	ImGui::DragFloat("softness", &effectData_->softness, 0.01f);
	ImGui::End();
#endif // _DEBUG
}

void VignetteEffect::Draw()
{
	dxManager_->GetCommandList()->SetPipelineState(psoManager_->GetOffScreenPSO(OffScreenEffectType::kVignette));
	dxManager_->GetCommandList()->SetGraphicsRootSignature(psoManager_->GetOffScreenSignature());
	dxManager_->GetCommandList()->SetGraphicsRootDescriptorTable(0, dxManager_->GetSrvHandle().second);

	dxManager_->GetCommandList()->SetGraphicsRootConstantBufferView(1, effectResource_->GetGPUVirtualAddress());

	dxManager_->GetCommandList()->DrawInstanced(3, 1, 0, 1);
}

void VignetteEffect::CreateEffectResource()
{
	// ヴィネット用のリソースを作る
	effectResource_ = dxManager_->CreateBufferResource(sizeof(VignetteEffectData));
	// 書き込むためのアドレスを取得
	effectResource_->Map(0, nullptr, reinterpret_cast<void**>(&effectData_));
	// 初期値を設定
	effectData_->radius = 0.5f;
	effectData_->intensity = 0.5f;
	effectData_->softness = -0.1f;
}
