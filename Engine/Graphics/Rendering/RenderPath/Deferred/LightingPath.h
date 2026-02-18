#pragma once
#include "Graphics/Device/DirectXManager.h"
#include "Graphics/Rendering/PSO/PSOManager.h"
#include <math/Vector2.h>
#include <math/Vector3.h>

class DirectXManager;
class GBufferManager;

struct FullScreenVertex {
	Vector3 pos;
	Vector2 uv;
};

class LightingPath
{
public:
	LightingPath() = default;
	~LightingPath();

	void Initialize(DirectXManager* dx, GBufferManager* gBuffer, PSOManager* psoManager);

	void CreateFullScreenVB();

	void Begin(uint32_t rtvIndex);
	void End();

	void CreateGBufferSRVs();
private:
	DirectXManager* dxManager_ = nullptr;
	GBufferManager* gBufferManager_ = nullptr;
	PSOManager* psoManager_ = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> fullScreenVB_{};
	D3D12_VERTEX_BUFFER_VIEW fullScreenVBV_{};

	uint32_t gBufferSrvStartIndex_;  // 先頭のSRV番号
	D3D12_GPU_DESCRIPTOR_HANDLE gBufferSrvTable_; // GPUハンドル保持
};

