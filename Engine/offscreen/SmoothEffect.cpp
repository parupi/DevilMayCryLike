#include "SmoothEffect.h"
#include "OffScreenManager.h"
#include <imgui/imgui.h>

SmoothEffect::SmoothEffect() 
{
	dxManager_ = OffScreenManager::GetInstance()->GetDXManager();
	psoManager_ = OffScreenManager::GetInstance()->GetPSOManager();

	CreateEffectResource();

	//isActive_ = true;
}

SmoothEffect::~SmoothEffect()
{
	// 借りてるポインタを破棄
	dxManager_ = nullptr;
	psoManager_ = nullptr;
	// 生成したリソースの削除
	effectData_ = nullptr;
	effectHandle_ = 0;
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
	dxManager_->GetCommandList()->SetGraphicsRootDescriptorTable(0, inputSrv_);

	dxManager_->GetCommandList()->SetGraphicsRootConstantBufferView(1, dxManager_->GetResourceManager()->GetGPUVirtualAddress(effectHandle_));

	dxManager_->GetCommandList()->DrawInstanced(3, 1, 0, 0);
}

void SmoothEffect::CreateEffectResource()
{
	auto* resourceManager = dxManager_->GetResourceManager();
	// ヴィネット用のリソースを作る
	effectHandle_ = resourceManager->CreateUploadBuffer(sizeof(SmoothEffectData), L"GaussianEffect");
	// 書き込むためのアドレスを取得
	effectData_ = reinterpret_cast<SmoothEffectData*>(resourceManager->Map(effectHandle_));
	// 初期値を設定
	effectData_->blurStrength = 1.0f;
	effectData_->iterations = 1;
}
