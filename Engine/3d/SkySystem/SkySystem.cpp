#include "SkySystem.h"
#include <3d/Object/Object3dManager.h>
#include <base/TextureManager.h>
#include <3d/Camera/CameraManager.h>


void SkySystem::Initialize(DirectXManager* dxManager, PSOManager* psoManager, SrvManager* srvManager)
{
	dxManager_ = dxManager;
	psoManager_ = psoManager;
	srvManager_ = srvManager;

	CreateSkyBoxVertex();
	CreateVertexResource();
	CreateIndexResource();

	TextureManager::GetInstance()->LoadTexture("skybox_cube.dds");

	MaterialData materialData;
	materialData.textureFilePath = "skybox_cube.dds";
	const DirectX::TexMetadata& meta = TextureManager::GetInstance()->GetMetaData("Resource/Images/skybox_cube.dds");

	Logger::Log(std::format("Cubemap?: {}\n", meta.IsCubemap() ? "Yes" : "No"));
	Logger::Log(std::format("Format: {}\n", static_cast<int>(meta.format)));

	materialData.textureIndex = TextureManager::GetInstance()->GetTextureIndexByFilePath("skybox_cube.dds");
	//materialData.textureIndex = TextureManager::GetInstance()->GetTextureIndexByFilePath("skybox_cube.dds");

	material_ = std::make_unique<Material>();
	material_->Initialize(dxManager_, srvManager_, materialData);



	// 定数バッファリソースの生成
	skyboxConstBuffer_ = dxManager_->CreateBufferResource(sizeof(SkyboxMatrix));
}

void SkySystem::Draw()
{
	//auto* commandList = dxManager_->GetCommandList();

	//// カメラ取得
	//Camera* camera = CameraManager::GetInstance()->GetActiveCamera();
	//Matrix4x4 view = camera->GetViewMatrix();
	//Matrix4x4 proj = camera->GetProjectionMatrix();

	//// 平行移動を除去
	//view.m[3][0] = 0.0f;
	//view.m[3][1] = 0.0f;
	//view.m[3][2] = 0.0f;

	//// 転送
	//skyboxMatrix_.viewProjectionNoTranslate = view * proj;

	//// 定数バッファ更新
	//void* mapped = nullptr;
	//skyboxConstBuffer_->Map(0, nullptr, &mapped);
	//memcpy(mapped, &skyboxMatrix_, sizeof(skyboxMatrix_));
	//skyboxConstBuffer_->Unmap(0, nullptr);


	//// PSOとルートシグネチャを設定
	//commandList->SetPipelineState(psoManager_->GetSkyboxPSO());
	//commandList->SetGraphicsRootSignature(psoManager_->GetSkyboxSignature());
	//commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	//// 頂点バッファ設定
	//commandList->IASetVertexBuffers(0, 1, &vertexBufferView_);

	//commandList->IASetIndexBuffer(&indexBufferView_);

	//commandList->SetGraphicsRootConstantBufferView(1, skyboxConstBuffer_->GetGPUVirtualAddress());

	//// SRV（キューブマップ）バインド
	//material_->Bind();

	//// 描画コール
	//commandList->DrawIndexedInstanced(static_cast<UINT>(indexData_.size()), 1, 0, 0, 0);
}

void SkySystem::CreateSkyBoxVertex()
{
	vertexData_.resize(8);

	auto Set = [](float x, float y, float z) -> VertexDataForSkybox {
		Vector3 pos = { x, y, z };
		Vector3 dir = Normalize(pos);
		return { {x, y, z, 1.0f}, dir };
		};

	// 右上前から時計回りに（Y-up）
	vertexData_[0] = Set(-1.0f, +1.0f, -1.0f); // 0: 左上奥
	vertexData_[1] = Set(+1.0f, +1.0f, -1.0f); // 1: 右上奥
	vertexData_[2] = Set(+1.0f, +1.0f, +1.0f); // 2: 右上前
	vertexData_[3] = Set(-1.0f, +1.0f, +1.0f); // 3: 左上前
	vertexData_[4] = Set(-1.0f, -1.0f, -1.0f); // 4: 左下奥
	vertexData_[5] = Set(+1.0f, -1.0f, -1.0f); // 5: 右下奥
	vertexData_[6] = Set(+1.0f, -1.0f, +1.0f); // 6: 右下前
	vertexData_[7] = Set(-1.0f, -1.0f, +1.0f); // 7: 左下前

	indexData_.resize(36);

	indexData_ = {
		// 前面 (+Z)
		3, 2, 6,
		3, 6, 7,

		// 背面 (-Z)
		1, 0, 4,
		1, 4, 5,

		// 右面 (+X)
		2, 1, 5,
		2, 5, 6,

		// 左面 (-X)
		0, 3, 7,
		0, 7, 4,

		// 上面 (+Y)
		0, 1, 2,
		0, 2, 3,

		// 下面 (-Y)
		7, 6, 5,
		7, 5, 4
	};
}

void SkySystem::CreateVertexResource()
{
	// バッファサイズの計算
	UINT sizeVB = static_cast<UINT>(sizeof(VertexDataForSkybox) * vertexData_.size());

	vertexResource_ = dxManager_->CreateBufferResource(sizeVB);

	// 頂点データの書き込み
	VertexDataForSkybox* vertexMap = nullptr;
	vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexMap));
	std::copy(vertexData_.begin(), vertexData_.end(), vertexMap);
	vertexResource_->Unmap(0, nullptr);

	// ビューの設定
	vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
	vertexBufferView_.SizeInBytes = sizeVB;
	vertexBufferView_.StrideInBytes = sizeof(VertexDataForSkybox);
}

void SkySystem::CreateIndexResource()
{
	// バッファサイズの計算（uint16_t型なので2バイト単位）
	UINT sizeIB = static_cast<UINT>(sizeof(uint16_t) * indexData_.size());

	// リソース生成
	indexResource_ = dxManager_->CreateBufferResource(sizeIB);

	// インデックスデータの書き込み
	uint16_t* indexMap = nullptr;
	indexResource_->Map(0, nullptr, reinterpret_cast<void**>(&indexMap));
	std::copy(indexData_.begin(), indexData_.end(), indexMap);
	indexResource_->Unmap(0, nullptr);

	// ビューの設定
	indexBufferView_.BufferLocation = indexResource_->GetGPUVirtualAddress();
	indexBufferView_.SizeInBytes = sizeIB;
	indexBufferView_.Format = DXGI_FORMAT_R16_UINT;  // 16bitインデックスの場合
}
