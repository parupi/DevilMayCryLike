#pragma once
#include "SpriteManager.h"
#include "base/TextureManager.h"
#include "math/Vector2.h"
#include "math/Vector3.h"
#include "math/Vector4.h"
#include "math/Matrix4x4.h"
#include "math/function.h"
class SpriteManager;
// スプライト
class Sprite {
public:
	Sprite() = default;
	~Sprite() = default;

	void Initialize(std::string textureFilePath);
	void Update();
	void Draw();

private:
	void CreateVertexResource();
	void CreateIndexResource();
	void CreateMaterialResource();
	void CreateTransformationResource();
	void SetSpriteData();

	void AdjustTextureSize();
private:

	// 構造体
	struct VertexData {
		Vector4 position;
		Vector2 texcoord;
	};

	struct Material {
		Vector4 color;
		Matrix4x4 uvTransform;
	};

	struct TransformationMatrix {
		Matrix4x4 WVP;
		Matrix4x4 World;
	};

	SpriteManager* spriteManager_ = nullptr;

	// バッファリソース
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> indexResource_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> transformationMatrixResource_ = nullptr;
	// バッファリソース内のデータを指すポインタ
	VertexData* vertexData_ = nullptr;
	uint32_t* indexData_ = nullptr;
	Material* materialData_ = nullptr;
	TransformationMatrix* transformationMatrixData_ = nullptr;
	// バッファリソースの使い道を補足するバッファビュー
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};
	D3D12_INDEX_BUFFER_VIEW indexBufferView_{};

	D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandleCPU_;
	D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU_;

	// テクスチャ番号
	uint32_t textureIndex = 0;
	std::string textureFilePath_;

	// 左右フリップ
	bool isFlipX_ = false;
	// 上下フリップ
	bool isFlipY_ = false;

private: // 実用化用変数
	EulerTransform transform_{ {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} };
	Vector2 position_ = { 0.0f, 0.0f };
	float rotation_ = 0.0f;
	Vector2 size_ = { 80.0f, 80.0f };

	EulerTransform uvTransform_{ {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} };
	Vector2 uvPosition_ = { 0.0f, 0.0f };
	float uvRotation_ = 0.0f;
	Vector2 uvSize_ = { 1.0f, 1.0f };

	// 中心点
	Vector2 anchorPoint_ = { 0.0f, 0.0f };
	// テクスチャ左上座標
	Vector2 textureLeftTop_ = { 0.0f, 0.0f };
	// テクスチャ切り出しサイズ
	Vector2 textureSize_ = { 100.0f, 100.0f };



public: // ゲッター//セッター //
	// 平行移動
	const Vector2& GetPosition() const { return position_; }
	void SetPosition(const Vector2& position) { position_ = position; }
	// 回転
	float GetRotation() const { return rotation_; }
	void SetRotation(float rotation) { rotation_ = rotation; }
	// 拡縮
	const Vector2& GetSize() const { return size_; }
	void SetSize(const Vector2& size) { size_ = size; }
	// 色
	const Vector4& GetColor() const { return materialData_->color; }
	void SetColor(const Vector4& color) { materialData_->color = color; }
	// アンカー
	const Vector2& GetAnchorPoint() const { return anchorPoint_; }
	void SetAnchorPoint(const Vector2& anchorPoint) { anchorPoint_ = anchorPoint; }
	// フリップX
	const bool& GetIsFlipX() const {return isFlipX_;}
	void SetIsFlipX(bool isFlipX) { isFlipX_ = isFlipX; }
	// フリップY
	const bool& GetIsFlipY() const { return isFlipY_; }
	void SetIsFlipY(bool isFlipY) { isFlipY_ = isFlipY; }
	// 左上座標
	const Vector2& GetTextureLeftTop() const { return textureLeftTop_; }
	void SetTextureLeftTop(Vector2 textureLeftTop) { textureLeftTop_ = textureLeftTop; }
	// 切り出しサイズ
	const Vector2& GetTextureSize() const { return textureSize_; }
	void SetTextureSize(Vector2 textureSize) { textureSize_ = textureSize; }
	// uv平行移動
	const Vector2& GetUVPosition() const { return uvPosition_; }
	void SetUVPosition(const Vector2& position) { uvPosition_ = position; }
	// 回転
	float GetUVRotation() const { return uvRotation_; }
	void SetUVRotation(float rotation) { uvRotation_ = rotation; }
	// 拡縮
	const Vector2& GetUVSize() const { return uvSize_; }
	void SetUVSize(const Vector2& size) { uvSize_ = size; }

}; 