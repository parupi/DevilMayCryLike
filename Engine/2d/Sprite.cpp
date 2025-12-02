#include "Sprite.h"
#include "math/function.h"
#ifdef _DEBUG
#include <imgui/imgui.h>
#endif _DEBUG

void Sprite::Initialize(std::string textureFilePath)
{
	spriteManager_ = SpriteManager::GetInstance();

	// 単位行列を書き込んでおく
	textureIndex = TextureManager::GetInstance()->GetTextureIndexByFilePath(textureFilePath);
	textureFilePath_ = textureFilePath;

	
	// 各種リソースを作る
	CreateVertexResource();
	CreateIndexResource();
	CreateMaterialResource();
	CreateTransformationResource();

	AdjustTextureSize();
}

void Sprite::Update()
{
	SetSpriteData();

	transform_.translate = { position_.x, position_.y, 0.0f };
	transform_.rotate = { 0.0f, 0.0f, rotation_ };
	transform_.scale = { size_.x, size_.y, 1.0f };

	// Transform情報を作る
	Matrix4x4 worldMatrix = MakeAffineMatrix(transform_.scale, transform_.rotate, transform_.translate);
	Matrix4x4 viewMatrix = MakeIdentity4x4();
	Matrix4x4 projectionMatrix = MakeOrthographicMatrix(0.0f, 0.0f, float(WindowManager::kClientWidth), float(WindowManager::kClientHeight), 0.0f, 100.0f);
	Matrix4x4 worldViewProjectionMatrix = worldMatrix * viewMatrix * projectionMatrix;
	transformationMatrixData_->World = worldViewProjectionMatrix;
	transformationMatrixData_->WVP = worldViewProjectionMatrix;

	uvTransform_.translate = { uvPosition_.x, uvPosition_.y, 0.0f };
	uvTransform_.rotate = { 0.0f, 0.0f, uvRotation_ };
	uvTransform_.scale = { uvSize_.x, uvSize_.y, 1.0f };

	// Transform情報を作る
	Matrix4x4 uvTransformMatrix = MakeIdentity4x4();
	uvTransformMatrix *= MakeScaleMatrix(uvTransform_.scale);
	uvTransformMatrix *= MakeRotateZMatrix(uvTransform_.rotate.z);
	uvTransformMatrix *= MakeTranslateMatrix(uvTransform_.translate);
	materialData_->uvTransform = uvTransformMatrix;
}

void Sprite::Draw()
{
	auto* resourceManager = spriteManager_->GetDxManager()->GetResourceManager();
	auto* commandList = spriteManager_->GetDxManager()->GetCommandList();

	commandList->SetGraphicsRootConstantBufferView(1, resourceManager->GetGPUVirtualAddress(transformHandle_));
	// Spriteの描画
	commandList->IASetVertexBuffers(0, 1, &vertexBufferView_);
	commandList->IASetIndexBuffer(&indexBufferView_);
	// TransformationMatrixCBufferの場所を設定
	commandList->SetGraphicsRootConstantBufferView(0, resourceManager->GetGPUVirtualAddress(materialHandle_));

	commandList->SetGraphicsRootDescriptorTable(2, TextureManager::GetInstance()->GetSrvHandleGPU(textureFilePath_));

	//// 描画!（DrawCall/ドローコール）。3頂点で1つのインスタンス。インスタンスについては今後
	commandList->DrawIndexedInstanced(6, 1, 0, 0, 0);
}

void Sprite::CreateVertexResource()
{
	auto* rm = spriteManager_->GetDxManager()->GetResourceManager();

	// ハンドル版 UploadBuffer を作成（CPUアクセス可能）
	vertexHandle_ = rm->CreateUploadBuffer(sizeof(VertexData) * 6, L"SpriteVertex");

	// CPU書き込み用アドレスを取得
	void* ptr = rm->Map(vertexHandle_);
	assert(ptr);

	vertexData_ = reinterpret_cast<VertexData*>(ptr);

	// GPU バッファビュー設定
	auto* res = rm->GetResource(vertexHandle_);
	vertexBufferView_.BufferLocation = res->GetGPUVirtualAddress();
	vertexBufferView_.SizeInBytes = sizeof(VertexData) * 6;
	vertexBufferView_.StrideInBytes = sizeof(VertexData);
}

void Sprite::CreateIndexResource()
{
	auto* rm = spriteManager_->GetDxManager()->GetResourceManager();

	indexHandle_ = rm->CreateUploadBuffer(sizeof(uint32_t) * 6, L"SpriteIndex");

	void* ptr = rm->Map(indexHandle_);
	assert(ptr);

	indexData_ = reinterpret_cast<uint32_t*>(ptr);

	auto* res = rm->GetResource(indexHandle_);
	indexBufferView_.BufferLocation = res->GetGPUVirtualAddress();
	indexBufferView_.SizeInBytes = sizeof(uint32_t) * 6;
	indexBufferView_.Format = DXGI_FORMAT_R32_UINT;
}

void Sprite::CreateMaterialResource()
{
	// Sprite用のマテリアルリソースを作る
	materialHandle_ = spriteManager_->GetDxManager()->GetResourceManager()->CreateUploadBuffer(sizeof(Material), L"SpriteMaterial");
	// 書き込むためのアドレスを取得
	void* ptr = spriteManager_->GetDxManager()->GetResourceManager()->Map(materialHandle_);
	assert(ptr);
	materialData_ = reinterpret_cast<Material*>(ptr);
	// 今回は白を書き込んで置く
	materialData_->color = { 1.0f, 1.0f, 1.0f, 1.0f };
	materialData_->uvTransform = MakeIdentity4x4();
}

void Sprite::CreateTransformationResource()
{
	auto* resourceManager = spriteManager_->GetDxManager()->GetResourceManager();
	// Sprite用のTransformationMatrix用のリソースを作る。Matrix4x4 1つ分のサイズを用意する
	transformHandle_ = resourceManager->CreateUploadBuffer(sizeof(TransformationMatrix), L"SpriteTransform");
	// 書き込むためのアドレスを取得
	void* ptr = resourceManager->Map(transformHandle_);
	assert(ptr);
	transformationMatrixData_ = reinterpret_cast<TransformationMatrix*>(ptr);
	// 単位行列を書き込んでおく
	transformationMatrixData_->World = MakeIdentity4x4();
	transformationMatrixData_->WVP = MakeIdentity4x4();
}

void Sprite::SetSpriteData()
{
	float left = 0.0f - anchorPoint_.x;
	float right = 1.0f - anchorPoint_.x;
	float top = 0.0f - anchorPoint_.y;
	float bottom = 1.0f - anchorPoint_.y;

	// 左右反転
	if (isFlipX_) {
		left = -left;
		right = -right;
	}
	// 上下反転
	if (isFlipY_) {
		top = -top;
		bottom = -bottom;
	}

	const DirectX::TexMetadata& metadata = TextureManager::GetInstance()->GetMetaData(textureFilePath_);
	float tex_left = textureLeftTop_.x / metadata.width;
	float tex_right = (textureLeftTop_.x + textureSize_.x) / metadata.width;
	float tex_top = textureLeftTop_.y / metadata.height;
	float tex_bottom = (textureLeftTop_.y + textureSize_.y) / metadata.height;

	// 左下
	vertexData_[0].position = { left, bottom, 0.0f, 1.0f };
	vertexData_[0].texcoord = { tex_left, tex_bottom };
	// 左上
	vertexData_[1].position = { left, top, 0.0f, 1.0f };
	vertexData_[1].texcoord = { tex_left, tex_top };
	// 右下
	vertexData_[2].position = { right, bottom, 0.0f, 1.0f };
	vertexData_[2].texcoord = { tex_right, tex_bottom };
	// 右上
	vertexData_[3].position = { right, top, 0.0f, 1.0f };
	vertexData_[3].texcoord = { tex_right, tex_top };

	indexData_[0] = 0;
	indexData_[1] = 1;
	indexData_[2] = 2;
	indexData_[3] = 1;
	indexData_[4] = 3;
	indexData_[5] = 2;
}

void Sprite::AdjustTextureSize()
{
	// テクスチャメタデータを取得
	const DirectX::TexMetadata& metadata = TextureManager::GetInstance()->GetMetaData(textureFilePath_);

	textureSize_.x = static_cast<float>(metadata.width);
	textureSize_.y = static_cast<float>(metadata.height);
	// 画像サイズをテクスチャサイズに合わせる
	size_ = textureSize_;
}
