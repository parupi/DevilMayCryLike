#include "GrayEffect.h"
#include <base/PSOManager.h>
#include "OffScreenManager.h"
#include <algorithm>
#include <imgui/imgui.h>

GrayEffect::GrayEffect() : BaseOffScreen()
{
	dxManager_ = OffScreenManager::GetInstance()->GetDXManager();
	psoManager_ = OffScreenManager::GetInstance()->GetPSOManager();

	CreateEffectResource();
}

void GrayEffect::Update()
{
#ifdef _DEBUG
	ImGui::Begin("GrayEffect");
	ImGui::Checkbox("isActive", &isActive_);
	ImGui::DragFloat("intensity", &effectData_->intensity, 0.01f);
	ImGui::End();

	//effectDclamp(effectData_->intensity, 0.0f, 1.0f);
#endif // _DEBUG
}

void GrayEffect::Draw()
{
	dxManager_->GetCommandList()->SetPipelineState(psoManager_->GetOffScreenPSO(OffScreenEffectType::kGray));
	dxManager_->GetCommandList()->SetGraphicsRootSignature(psoManager_->GetOffScreenSignature());
	dxManager_->GetCommandList()->SetGraphicsRootDescriptorTable(0, dxManager_->GetSrvHandle().second);

	dxManager_->GetCommandList()->SetGraphicsRootConstantBufferView(1, effectResource_->GetGPUVirtualAddress());

	dxManager_->GetCommandList()->DrawInstanced(3, 1, 0, 1);
}

void GrayEffect::CreateEffectResource()
{
	// グレイスケール用のリソースを作る
	effectResource_ = dxManager_->CreateBufferResource(sizeof(GrayEffectData));
	// 書き込むためのアドレスを取得
	effectResource_->Map(0, nullptr, reinterpret_cast<void**>(&effectData_));
	// 初期値を設定
	effectData_->intensity = 1.0f;
}
