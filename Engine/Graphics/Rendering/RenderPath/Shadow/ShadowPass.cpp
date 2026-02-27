#include "ShadowPass.h"
#include "Graphics/Device/DirectXManager.h"
#include "Graphics/Rendering/Shadow/CascadedShadowMap.h"
#include "Graphics/Rendering/PSO/PSOManager.h"
#include "Graphics/Resource/GpuResourceFactory.h"
#include <3d/Object/Object3dManager.h>

void ShadowPass::Initialize(DirectXManager* dxManager, PSOManager* psoManager, CascadedShadowMap* shadowMap)
{
	dxManager_ = dxManager;
	psoManager_ = psoManager;
	shadowMap_ = shadowMap;

	CreateResource();
}

void ShadowPass::BeginDraw()
{
	auto* commandContext = dxManager_->GetCommandContext();

	commandContext->TransitionResource(
		depthBuffer_.Get(),
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_DEPTH_WRITE
	);
	
	D3D12_VIEWPORT vp{};
	vp.Width = (float)shadowMap_->GetShadowMapSize();
	vp.Height = (float)shadowMap_->GetShadowMapSize();
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;

	D3D12_RECT scissor{ 0, 0, (LONG)shadowMap_->GetShadowMapSize(), (LONG)shadowMap_->GetShadowMapSize() };

	commandContext->SetViewportAndScissor(vp, scissor);

	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dxManager_->GetDsvManager()->GetDsvHandle(dsvIndex_);
	commandContext->SetRenderTargets(nullptr, 0, &dsvHandle);

	commandContext->GetCommandList()->SetPipelineState(psoManager_->GetCSMPSO());
	commandContext->GetCommandList()->SetGraphicsRootSignature(psoManager_->GetCSMRootSignature());
	commandContext->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void ShadowPass::Execute()
{
	for (uint32_t i = 0; i < kCascadeCount; ++i) {
		shadowMap_->BeginCascade(i);

		shadowMap_->Bind(1, i);

		// ここで影を落とすオブジェクトを描画
		Object3dManager::GetInstance()->DrawShadow();

		shadowMap_->EndCascade(i);
	}
}

void ShadowPass::EndDraw()
{
	auto* commandContext = dxManager_->GetCommandContext();

	commandContext->TransitionResource(
		depthBuffer_.Get(),
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
	);
}

void ShadowPass::CreateResource()
{
	// DSV生成
	GpuResourceFactory::TextureDesc dsvDesc{};
	dsvDesc.format = DXGI_FORMAT_R24G8_TYPELESS;
	dsvDesc.usage = GpuResourceFactory::Usage::DepthStencil;
	dsvDesc.clearDepth = 1.0f;

	depthBuffer_ = dxManager_->GetResourceFactory()->CreateTexture2D(dsvDesc);
	depthBuffer_->SetName(L"DepthBufferForCSM");

	auto* dsvManager = dxManager_->GetDsvManager();
	dsvIndex_ = dsvManager->Allocate();
	dsvManager->CreateDsv(dsvIndex_, depthBuffer_.Get(), DXGI_FORMAT_D24_UNORM_S8_UINT);

	dxManager_->GetCommandContext()->TransitionResource(
		depthBuffer_.Get(),
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
	);

	// SRV の生成
	srvIndex_ = dxManager_->GetSrvManager()->Allocate();
	dxManager_->GetSrvManager()->CreateSRVforTexture2D(srvIndex_, depthBuffer_.Get(), DXGI_FORMAT_R24_UNORM_X8_TYPELESS, 1);
}
