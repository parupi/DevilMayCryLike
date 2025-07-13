#include "SkySystem.h"
#include <3d/Object/Object3dManager.h>
#include <base/TextureManager.h>


void SkySystem::Initialize(DirectXManager* dxManager, PSOManager* psoManager, SrvManager* srvManager)
{


	dxManager_ = dxManager;
	psoManager_ = psoManager;
	srvManager_ = srvManager;

	CreateSkyBoxVertex();
	CreateVertexResource();

	TextureManager::GetInstance()->LoadTexture("rostock_laage_airport_4k.dds");

	MaterialData materialData;
	materialData.textureFilePath = "rostock_laage_airport_4k.dds";
	materialData.textureIndex = TextureManager::GetInstance()->GetTextureIndexByFilePath("rostock_laage_airport_4k.dds");

	material_ = std::make_unique<Material>();
	material_->Initialize(dxManager_, srvManager_, materialData);

	transform_ = std::make_unique<WorldTransform>();
	transform_->Initialize();
	transform_->GetScale() = { 100.0f, 100.0f, 100.0f };
	transform_->TransferMatrix();
}

void SkySystem::Draw()
{
	//auto* commandList = dxManager_->GetCommandList();

	//// PSOとルートシグネチャを設定
	//commandList->SetPipelineState(psoManager_->GetSkyboxPSO());
	//commandList->SetGraphicsRootSignature(psoManager_->GetSkyboxSignature());
	//commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	//// 頂点バッファ設定
	//commandList->IASetVertexBuffers(0, 1, &vertexBufferView_);

	//commandList->SetGraphicsRootConstantBufferView(1, transform_->GetConstBuffer()->GetGPUVirtualAddress());

	//// SRV（キューブマップ）バインド
	////srvManager_->SetGraphicsRootDescriptorTable(2, textureIndex_);
	//material_->Bind();


	//
	//// 描画コール
	//commandList->DrawInstanced(36, 1, 0, 0);
}

void SkySystem::CreateSkyBoxVertex()
{
	// 頂点数を必要分設定（6面 × 2三角形 × 3頂点 = 36）
	vertexData_.resize(36);

	// 右面 (+X)
	vertexData_[0].position = { 1.0f, 1.0f, 1.0f, 1.0f };
	vertexData_[1].position = { 1.0f, 1.0f, -1.0f, 1.0f };
	vertexData_[2].position = { 1.0f, -1.0f, 1.0f, 1.0f };
	vertexData_[3].position = { 1.0f, -1.0f, 1.0f, 1.0f };
	vertexData_[4].position = { 1.0f, 1.0f, -1.0f, 1.0f };
	vertexData_[5].position = { 1.0f, -1.0f, -1.0f, 1.0f };

	// 左面 (-X)
	vertexData_[6].position = { -1.0f, 1.0f, -1.0f, 1.0f };
	vertexData_[7].position = { -1.0f, 1.0f, 1.0f, 1.0f };
	vertexData_[8].position = { -1.0f, -1.0f, -1.0f, 1.0f };
	vertexData_[9].position = { -1.0f, -1.0f, -1.0f, 1.0f };
	vertexData_[10].position = { -1.0f, 1.0f, 1.0f, 1.0f };
	vertexData_[11].position = { -1.0f, -1.0f, 1.0f, 1.0f };

	// 前面 (+Z)
	vertexData_[12].position = { -1.0f, 1.0f, 1.0f, 1.0f };
	vertexData_[13].position = { 1.0f, 1.0f, 1.0f, 1.0f };
	vertexData_[14].position = { -1.0f, -1.0f, 1.0f, 1.0f };
	vertexData_[15].position = { -1.0f, -1.0f, 1.0f, 1.0f };
	vertexData_[16].position = { 1.0f, 1.0f, 1.0f, 1.0f };
	vertexData_[17].position = { 1.0f, -1.0f, 1.0f, 1.0f };

	// 背面 (-Z)
	vertexData_[18].position = { 1.0f, 1.0f, -1.0f, 1.0f };
	vertexData_[19].position = { -1.0f, 1.0f, -1.0f, 1.0f };
	vertexData_[20].position = { 1.0f, -1.0f, -1.0f, 1.0f };
	vertexData_[21].position = { 1.0f, -1.0f, -1.0f, 1.0f };
	vertexData_[22].position = { -1.0f, 1.0f, -1.0f, 1.0f };
	vertexData_[23].position = { -1.0f, -1.0f, -1.0f, 1.0f };

	// 上面 (+Y)
	vertexData_[24].position = { -1.0f, 1.0f, -1.0f, 1.0f };
	vertexData_[25].position = { 1.0f, 1.0f, -1.0f, 1.0f };
	vertexData_[26].position = { -1.0f, 1.0f, 1.0f, 1.0f };
	vertexData_[27].position = { -1.0f, 1.0f, 1.0f, 1.0f };
	vertexData_[28].position = { 1.0f, 1.0f, -1.0f, 1.0f };
	vertexData_[29].position = { 1.0f, 1.0f, 1.0f, 1.0f };

	// 下面 (-Y)
	vertexData_[30].position = { -1.0f, -1.0f, 1.0f, 1.0f };
	vertexData_[31].position = { 1.0f, -1.0f, 1.0f, 1.0f };
	vertexData_[32].position = { -1.0f, -1.0f, -1.0f, 1.0f };
	vertexData_[33].position = { -1.0f, -1.0f, -1.0f, 1.0f };
	vertexData_[34].position = { 1.0f, -1.0f, 1.0f, 1.0f };
	vertexData_[35].position = { 1.0f, -1.0f, -1.0f, 1.0f };

	//for (int32_t i = 0; i < 36; i++) {
	//	vertexData_[i].normal = { 1.0f, 1.0f, 1.0f };
	//}
}

void SkySystem::CreateVertexResource()
{
	// バッファサイズの計算
	UINT sizeVB = static_cast<UINT>(sizeof(VertexData) * vertexData_.size());

	vertexResource_ = dxManager_->CreateBufferResource(sizeVB);

	// 頂点データの書き込み
	VertexData* vertexMap = nullptr;
	vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexMap));
	std::copy(vertexData_.begin(), vertexData_.end(), vertexMap);
	vertexResource_->Unmap(0, nullptr);

	// ビューの設定
	vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
	vertexBufferView_.SizeInBytes = sizeVB;
	vertexBufferView_.StrideInBytes = sizeof(VertexData);
}
