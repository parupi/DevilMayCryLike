#include "SkinningRenderPass.h"
#include "Graphics/Device/DirectXManager.h"
#include "World3D/Object/Object3dManager.h"

void SkinningRenderPass::Initialize(const EngineContext& ctx) {
	ctx_ = ctx;
}

void SkinningRenderPass::Execute() {
	// このパスが描画パスの先頭なので、SRVヒープをここでバインドしておく。
	// パレット等をルートデスクリプタテーブルで渡すため、これが無いと何も見えない
	ctx_.dxManager->GetSrvManager()->BeginDraw();

	ctx_.object3dManager->DispatchSkinning();
}
