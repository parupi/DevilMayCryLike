#include "GBufferRenderPass.h"
#include "Graphics/Device/DirectXManager.h"
#include "Graphics/Rendering/PSO/PSOManager.h"
#include "World3D/Object/Object3dManager.h"

void GBufferRenderPass::Initialize(const EngineContext& ctx, GBufferManager* gBufferManager,
	const SharedRenderTarget& sharedRT)
{
	ctx_ = ctx;
	sharedRT_ = sharedRT;

	gBufferPath_ = std::make_unique<GBufferPath>();
	gBufferPath_->Initialize(ctx_.dxManager, gBufferManager, ctx_.psoManager);
}

void GBufferRenderPass::Execute()
{
	auto* commandCtx = ctx_.dxManager->GetCommandContext();

	commandCtx->TransitionResource(sharedRT_.rtvResource,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
	commandCtx->TransitionResource(sharedRT_.depthBuffer,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE);

	gBufferPath_->Begin(sharedRT_.dsvIndex);
	ctx_.object3dManager->DrawDeferred();
	gBufferPath_->End();

	commandCtx->TransitionResource(sharedRT_.rtvResource,
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	commandCtx->TransitionResource(sharedRT_.depthBuffer,
		D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}
