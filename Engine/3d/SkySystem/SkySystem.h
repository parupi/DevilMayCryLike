#pragma once
#include "3d/Object/Model/ModelStructs.h"
#include <memory>
#include "base/PSOManager.h"
#include "base/DirectXManager.h"
#include "base/SrvManager.h"
#include "3d/Object/Model/Material/Material.h"
#include <3d/WorldTransform.h>

struct VertexDataForSkybox {
	Vector4 position{};
	Vector3 texcoord{};
};

struct SkyboxMatrix {
	Matrix4x4 viewProjectionNoTranslate;
};

class SkySystem
{
public:
	void Initialize(DirectXManager* dxManager, PSOManager* psoManager, SrvManager* srvManager);


	void Draw();

private:
	void CreateSkyBoxVertex();
	void CreateVertexResource();
	void CreateIndexResource();

	DirectXManager* dxManager_;
	PSOManager* psoManager_;
	SrvManager* srvManager_;
	
	std::vector<VertexDataForSkybox> vertexData_;
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView_;

	std::vector<uint16_t> indexData_;
	Microsoft::WRL::ComPtr<ID3D12Resource> indexResource_;
	D3D12_INDEX_BUFFER_VIEW indexBufferView_;

	int32_t textureIndex_ = 0;

	std::unique_ptr<Material> material_;
	// カメラのデータを送る
	Microsoft::WRL::ComPtr<ID3D12Resource> skyboxConstBuffer_;
	SkyboxMatrix skyboxMatrix_;  // CPU 側データ
};

