#include "Sprite.h"
#include "function.h"
#ifdef _DEBUG
#include <imgui.h>
#endif _DEBUG

Sprite::~Sprite() {

}

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
	spriteManager_->GetDxManager()->GetCommandList()->SetGraphicsRootConstantBufferView(1, transformationMatrixResource_->GetGPUVirtualAddress());
	// Spriteの描画。変更が必要な物だけ変更する
	spriteManager_->GetDxManager()->GetCommandList()->IASetVertexBuffers(0, 1, &vertexBufferView_);
	spriteManager_->GetDxManager()->GetCommandList()->IASetIndexBuffer(&indexBufferView_);
	// TransformationMatrixCBufferの場所を設定
	spriteManager_->GetDxManager()->GetCommandList()->SetGraphicsRootConstantBufferView(0, materialResource_->GetGPUVirtualAddress());

	spriteManager_->GetDxManager()->GetCommandList()->SetGraphicsRootDescriptorTable(2, TextureManager::GetInstance()->GetSrvHandleGPU(textureFilePath_));

	//// 描画!（DrawCall/ドローコール）。3頂点で1つのインスタンス。インスタンスについては今後
	spriteManager_->GetDxManager()->GetCommandList()->DrawIndexedInstanced(6, 1, 0, 0, 0);
}

void Sprite::CreateVertexResource()
{
	// Sprite用の頂点リソースを作る
	vertexResource_ = spriteManager_->GetDxManager()->CreateBufferResource(sizeof(VertexData) * 6);
	// リソースの先頭アドレスから使う
	vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
	// 使用するリソースのサイズは頂点6つ分のサイズ
	vertexBufferView_.SizeInBytes = sizeof(VertexData) * 6;
	// 1頂点当たりのサイズ
	vertexBufferView_.StrideInBytes = sizeof(VertexData);
}

void Sprite::CreateIndexResource()
{
	// Sprite用のリソースインデックスの作成
	indexResource_ = spriteManager_->GetDxManager()->CreateBufferResource(sizeof(uint32_t) * 6);
	// リソースの先頭のアドレスから使う
	indexBufferView_.BufferLocation = indexResource_->GetGPUVirtualAddress();
	// 使用するリソースのサイズはインデックス6つ分のサイズ
	indexBufferView_.SizeInBytes = sizeof(uint32_t) * 6;
	// インデックスはuint32_tとする
	indexBufferView_.Format = DXGI_FORMAT_R32_UINT;
}

void Sprite::CreateMaterialResource()
{
	// Sprite用のマテリアルリソースを作る
	materialResource_ = spriteManager_->GetDxManager()->CreateBufferResource(sizeof(Material));
	// 書き込むためのアドレスを取得
	materialResource_->Map(0, nullptr, reinterpret_cast<void**>(&materialData_));
	// 今回は白を書き込んで置く
	materialData_->color = { 1.0f, 1.0f, 1.0f, 1.0f };
	materialData_->uvTransform = MakeIdentity4x4();
}

void Sprite::CreateTransformationResource()
{
	// Sprite用のTransformationMatrix用のリソースを作る。Matrix4x4 1つ分のサイズを用意する
	transformationMatrixResource_ = spriteManager_->GetDxManager()->CreateBufferResource(sizeof(TransformationMatrix));
	// 書き込むためのアドレスを取得
	transformationMatrixResource_->Map(0, nullptr, reinterpret_cast<void**>(&transformationMatrixData_));
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

	vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexData_));
	// スプライトの描画
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

	// 書き込むためのアドレスを取得
	indexResource_->Map(0, nullptr, reinterpret_cast<void**>(&indexData_));
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
