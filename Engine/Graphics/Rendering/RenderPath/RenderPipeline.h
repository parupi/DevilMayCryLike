#pragma once
#include "IRenderPass.h"
#include "Impl/GBufferManager.h"
#include "Core/EngineContext.h"
#include <wrl.h>
#include <d3d12.h>
#include <vector>
#include <memory>

class RenderPipeline {
public:
	RenderPipeline() = default;
	~RenderPipeline() = default;

	void Initialize(const EngineContext& ctx);
	void Finalize();
	void Execute();

private:
	EngineContext ctx_{};

	// GBufferは複数パス(GBuffer/Lighting)で共有するため直接所有
	std::unique_ptr<GBufferManager> gBufferManager_;

	// 実行順に並べた描画パス
	std::vector<std::unique_ptr<IRenderPass>> passes_;

	// 全パスで共有するRTV/DSVリソース
	Microsoft::WRL::ComPtr<ID3D12Resource> rtvResource_;
	Microsoft::WRL::ComPtr<ID3D12Resource> depthBuffer_;
	SharedRenderTarget sharedRT_{};
	uint32_t srvIndex_ = 0;
};
