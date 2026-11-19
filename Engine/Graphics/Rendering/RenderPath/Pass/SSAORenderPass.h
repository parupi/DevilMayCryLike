#pragma once
#include "../IRenderPass.h"
#include "Core/EngineContext.h"
#include <wrl.h>
#include <d3d12.h>
#include "Graphics/Resource/ResourceManager.h"

class GBufferManager;

/// <summary>
/// SSAO。GBuffer の法線とワールド座標から、物の隙間・接地部分がどれだけ周りに遮られているかを求め、
/// 深度を見ながらぼかして AO テクスチャ（R8、1=遮られていない）にする。
/// </summary>
class SSAORenderPass : public IRenderPass {
public:
	void Initialize(const EngineContext& ctx, GBufferManager* gBufferManager);
	void Execute() override;
	const char* GetName() const override { return "SSAO"; }

private:
	struct Target {
		Microsoft::WRL::ComPtr<ID3D12Resource> resource;
		D3D12_CPU_DESCRIPTOR_HANDLE rtv{};
		D3D12_GPU_DESCRIPTOR_HANDLE srv{};
	};

	// HLSL の SSAOParam と並びを合わせること（SSAO.PS.hlsl）
	struct SSAOParam {
		float viewProj[16];
		float cameraPos[3];
		float radius;
		float bias;
		float intensity;
		float power;
		float _pad0;
	};
	// HLSL の SSAOBlurParam と並びを合わせること（SSAOBlur.PS.hlsl）
	struct BlurParam {
		float depthSharpness; // 距離が違う画素をどれだけ混ぜないか
		float _pad[3];
	};

	void CreateTargets();
	void CreateRootSignature();
	void CreatePSOs();
	void DrawFullscreen(ID3D12PipelineState* pso, const Target& output, D3D12_GPU_VIRTUAL_ADDRESS cbv,
		D3D12_GPU_DESCRIPTOR_HANDLE t0, D3D12_GPU_DESCRIPTOR_HANDLE t1);

	EngineContext ctx_{};
	GBufferManager* gBufferManager_ = nullptr;

	Target ao_{};   // 計算結果（ノイズが残っている）
	Target blur_{}; // ぼかした結果。ライティングが読むのはこちら

	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> ssaoPso_;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> blurPso_;

	BufferHandle ssaoParamHandle_ = kInvalidBufferHandle;
	SSAOParam* ssaoParam_ = nullptr;
	BufferHandle blurParamHandle_ = kInvalidBufferHandle;
	BlurParam* blurParam_ = nullptr;

	D3D12_VIEWPORT viewport_{};
	D3D12_RECT scissor_{};
};
