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

class SkySystem
{
private:
	static SkySystem* instance;
	static std::once_flag initInstanceFlag;

	SkySystem() = default;
	~SkySystem() = default;
	SkySystem(SkySystem&) = default;
	SkySystem& operator=(SkySystem&) = default;
public:
	// インスタンスの取得
	static SkySystem* GetInstance();
	// 初期化
	void Initialize(DirectXManager* dxManager, PSOManager* psoManager, SrvManager* srvManager);
	// 終了
	void Finalize();

	void Draw();

	// スカイボックス生成
	void CreateSkyBox(const std::string& textureFilePath);

	// テクスチャインデックスを取得
	int32_t GetEnvironmentMapIndex() const { return textureIndex_; }

private:
	// VertexData生成
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

	int32_t textureIndex_ = -1;

	std::unique_ptr<Material> material_;

	std::unique_ptr<WorldTransform> transform_;
};

