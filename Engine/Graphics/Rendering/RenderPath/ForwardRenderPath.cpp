#include "ForwardRenderPath.h"
#include "Graphics/Device/DirectXManager.h"
#include "Graphics/Rendering/PSO/PSOManager.h"
#include "Graphics/Resource/GpuResourceFactory.h"

void ForwardRenderPath::Initialize(DirectXManager* dxManager, PSOManager* psoManager)
{
	dxManager_ = dxManager;
	psoManager_ = psoManager;
}

void ForwardRenderPath::BeginDraw(uint32_t rtvIndex, uint32_t dsvIndex)
{
	auto cmd = dxManager_->GetCommandList();
	auto* commandContext = dxManager_->GetCommandContext();

	// パイプラインのセット
	cmd->SetPipelineState(psoManager_->GetObjectPSO(BlendMode::kNormal));
	cmd->SetGraphicsRootSignature(psoManager_->GetObjectSignature());

	// 描画ターゲットの設定
	commandContext->SetRenderTarget(dxManager_->GetRtvManager()->GetCPUDescriptorHandle(rtvIndex), dxManager_->GetDsvManager()->GetDsvHandle(dsvIndex));

	// depthのクリア
	//commandContext->ClearDepth(dxManager_->GetDsvManager()->GetDsvHandle(dsvIndex));
	// rtvのクリア
	//float clearColor[4] = { 0.6f, 0.5f, 0.1f, 1.0f };
	//commandContext->ClearRenderTarget(dxManager_->GetRtvManager()->GetCPUDescriptorHandle(rtvIndex), clearColor);

	ID3D12DescriptorHeap* heaps[] = { dxManager_->GetSrvManager()->GetHeap() };
	dxManager_->GetCommandList()->SetDescriptorHeaps(_countof(heaps), heaps);
}

void ForwardRenderPath::EndDraw()
{

}
