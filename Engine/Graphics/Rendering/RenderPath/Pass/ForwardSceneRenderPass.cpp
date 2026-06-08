#include "ForwardSceneRenderPass.h"
#include "Graphics/Device/DirectXManager.h"
#include "Graphics/Rendering/PSO/PSOManager.h"
#include "Scene/SceneManager.h"
#include "World3D/Object/Object3dManager.h"
#include "World3D/Collider/CollisionManager.h"
#include "World3D/Light/LightManager.h"
#include "Graphics/Rendering/Sky/SkySystem.h"
#include "Graphics/Rendering/Sprite/SpriteManager.h"
#include "Scene/Transition/TransitionManager.h"
#include "World3D/Primitive/PrimitiveLineDrawer.h"

void ForwardSceneRenderPass::Initialize(const EngineContext& ctx, const SharedRenderTarget& sharedRT) {
	ctx_ = ctx;
	sharedRT_ = sharedRT;

	forwardPath_ = std::make_unique<ForwardRenderPath>();
	forwardPath_->Initialize(ctx_.dxManager, ctx_.psoManager);
}

void ForwardSceneRenderPass::Execute() {
	auto* commandCtx = ctx_.dxManager->GetCommandContext();

	commandCtx->TransitionResource(sharedRT_.rtvResource, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
	commandCtx->TransitionResource(sharedRT_.depthBuffer, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE);

	forwardPath_->BeginDraw(sharedRT_.rtvIndex, sharedRT_.dsvIndex);

	ctx_.skySystem->Draw();
	ctx_.object3dManager->DrawForward();
	ctx_.sceneManager->Draw();
	ctx_.spriteManager->DrawAllSprite();
	ctx_.transitionManager->Draw();
	ctx_.collisionManager->Draw();
#ifdef _DEBUG
	ctx_.lightManager->DrawDebug();
#endif
	ctx_.primitiveLineDrawer->EndDraw();

	forwardPath_->EndDraw();

	ctx_.primitiveLineDrawer->BeginDraw();

	commandCtx->TransitionResource(sharedRT_.rtvResource,
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	commandCtx->TransitionResource(sharedRT_.depthBuffer,
		D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}
