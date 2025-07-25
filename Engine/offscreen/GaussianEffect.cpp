#include "GaussianEffect.h"
#include "OffScreenManager.h"
#include <imgui.h>

GaussianEffect::GaussianEffect()
{
	dxManager_ = OffScreenManager::GetInstance()->GetDXManager();
	psoManager_ = OffScreenManager::GetInstance()->GetPSOManager();

	CreateEffectResource();

	//isActive_ = true;
}

GaussianEffect::~GaussianEffect()
{
	// 借りてるポインタを破棄
	dxManager_ = nullptr;
	psoManager_ = nullptr;
	// 生成したリソースの削除
	effectData_ = nullptr;
	effectResource_.Reset();
}

void GaussianEffect::Update()
{
#ifdef _DEBUG
	ImGui::Begin("GaussianEffect");
	ImGui::Checkbox("isActive", &isActive_);

	ImGui::SliderFloat("Sigma", &effectData_->sigma, 0.1f, 10.0f, "%.2f");
	ImGui::SliderFloat("Blur Strength", &effectData_->blurStrength, 0.0f, 5.0f, "%.2f");
	ImGui::Combo("Alpha Mode", reinterpret_cast<int*>(&effectData_->alphaMode), "Fixed 1.0\0Sample Alpha\0\0");
	ImGui::DragFloat2("UV Clamp Min", &effectData_->uvClampMin.x, 0.01f, -1.0f, 1.0f, "%.2f");
	ImGui::DragFloat2("UV Clamp Max", &effectData_->uvClampMax.x, 0.01f, -1.0f, 1.0f, "%.2f");

	ImGui::End();
#endif // _DEBUG
}

void GaussianEffect::Draw()
{
	dxManager_->GetCommandList()->SetPipelineState(psoManager_->GetOffScreenPSO(OffScreenEffectType::kGauss));
	dxManager_->GetCommandList()->SetGraphicsRootSignature(psoManager_->GetOffScreenSignature());
	dxManager_->GetCommandList()->SetGraphicsRootDescriptorTable(0, inputSrv_);

	dxManager_->GetCommandList()->SetGraphicsRootConstantBufferView(1, effectResource_->GetGPUVirtualAddress());

	dxManager_->GetCommandList()->DrawInstanced(3, 1, 0, 0);
}

void GaussianEffect::CreateEffectResource()
{
	// ヴィネット用のリソースを作る
	dxManager_->CreateBufferResource(sizeof(GaussianEffectData), effectResource_);
	// 書き込むためのアドレスを取得
	effectResource_->Map(0, nullptr, reinterpret_cast<void**>(&effectData_));
	// 初期値を設定
	effectData_->sigma = 10.0f;
	effectData_->blurStrength = 1.0f;
	effectData_->alphaMode = 1.0f;
	effectData_->uvClampMin = { -1.0f, -1.0f };
	effectData_->uvClampMax = { 1.0f, 1.0f };
}
