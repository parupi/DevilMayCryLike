#pragma once
#include "Graphics/Rendering/RenderPath/ForwardRenderPath.h"
#include "Graphics/Rendering/RenderPath/Deferred/GBufferPath.h"
#include "Graphics/Rendering/RenderPath/Deferred/LightingPath.h"
#include "Graphics/Rendering/RenderPath/Deferred/GBufferManager.h"
#include "Graphics/Rendering/RenderPath/CompositePath.h"

class DirectXManager;
class PSOManager;

class RenderPipeline
{
public:
	RenderPipeline() = default;
	~RenderPipeline() = default;
	// 初期化
	void Initialize(DirectXManager* dxManager, PSOManager* psoManager);
	// 終了
	void Finalize();
	// 描画実行
	void Execute(PSOManager* psoManager);
	// ResourceをRTVに変更
	void TransitionToRTV();
	// ResourceをSRVに変更
	void TransitionToSRV();
	
private:
	DirectXManager* dxManager_ = nullptr;

	std::unique_ptr<GBufferManager> gBufferManager = nullptr;
	std::unique_ptr<GBufferPath> gBufferPath = nullptr;
	std::unique_ptr<LightingPath> lightingPath = nullptr;
	std::unique_ptr<ForwardRenderPath> forwardPath = nullptr;
	std::unique_ptr<CompositePath> compositePath = nullptr;

	// 全部のパスで書き込みをするRtv
	Microsoft::WRL::ComPtr<ID3D12Resource> rtvResource_;
	uint32_t rtvIndex_;
	// 全部のパスで書き込みをするRtv
	Microsoft::WRL::ComPtr<ID3D12Resource> srvResource_;
	uint32_t srvIndex_; 

	Microsoft::WRL::ComPtr<ID3D12Resource> depthBuffer_;
	uint32_t dsvIndex_ = 0;
};

