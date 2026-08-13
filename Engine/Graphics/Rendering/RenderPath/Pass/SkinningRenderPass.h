#pragma once
#include "Graphics/Rendering/RenderPath/IRenderPass.h"
#include "Core/EngineContext.h"

// スキンモデルの頂点をComputeShaderで変形させるパス。
// 影(CSM)・GBuffer・Forward がどれも同じ出力頂点バッファを読むため、
// それらより前に置いてフレームに1回だけ実行する
class SkinningRenderPass : public IRenderPass {
public:
	void Initialize(const EngineContext& ctx);
	void Execute() override;
	const char* GetName() const override { return "Skinning"; }

private:
	EngineContext ctx_{};
};
