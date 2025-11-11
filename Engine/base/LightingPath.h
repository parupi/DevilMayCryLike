#pragma once
#include <base/DirectXManager.h>
#include <base/PSOManager.h>
#include <base/GBufferManager.h>
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
	~LightingPath() = default;

	void Initialize(DirectXManager* dx, GBufferManager* gBuffer, PSOManager* psoManager);

	void CreateFullScreenVB();

	void Begin();
	void DrawDirectionalLight();
	void End();

private:
	DirectXManager* dxManager_ = nullptr;
	GBufferManager* gBufferManager_ = nullptr;
	PSOManager* psoManager_ = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> fullScreenVB_{};
	D3D12_VERTEX_BUFFER_VIEW fullScreenVBV_{};
};

