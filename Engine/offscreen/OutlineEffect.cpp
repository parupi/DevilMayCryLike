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
	effectResource_.Reset();
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

	dxManager_->GetCommandList()->SetGraphicsRootConstantBufferView(1, effectResource_->GetGPUVirtualAddress());

	dxManager_->GetCommandList()->DrawInstanced(3, 1, 0, 0);
}

void OutlineEffect::CreateEffectResource()
{
	// ヴィネット用のリソースを作る
	dxManager_->CreateBufferResource(sizeof(OutlineEffectData), effectResource_);
	// 書き込むためのアドレスを取得
	effectResource_->Map(0, nullptr, reinterpret_cast<void**>(&effectData_));
	// 初期値を設定

	effectResource_->Unmap(0, nullptr);
}
