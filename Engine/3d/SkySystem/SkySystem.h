#pragma once
#include "3d/Object/Model/ModelStructs.h"
#include <memory>
#include "base/PSOManager.h"
#include "base/DirectXManager.h"
#include "base/SrvManager.h"
#include "3d/Object/Model/Material/Material.h"
#include <3d/WorldTransform.h>
class SkySystem
{
public:
	void Initialize(DirectXManager* dxManager, PSOManager* psoManager, SrvManager* srvManager);


	void Draw();

private:
	void CreateSkyBoxVertex();
	void CreateVertexResource();

	DirectXManager* dxManager_;
	PSOManager* psoManager_;
	SrvManager* srvManager_;
	
	std::vector<VertexData> vertexData_;
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView_;

	int32_t textureIndex_ = 0;

	std::unique_ptr<Material> material_;
	std::unique_ptr<WorldTransform> transform_;
};

