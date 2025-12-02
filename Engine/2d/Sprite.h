#pragma once
#include "SpriteManager.h"
#include "base/TextureManager.h"
#include "math/Vector2.h"
#include "math/Vector3.h"
#include "math/Vector4.h"
#include "math/Matrix4x4.h"
#include "math/function.h"

class SpriteManager;

/// <summary>
/// 2Dスプライトを描画するためのクラス  
/// テクスチャを矩形ポリゴンに貼り付けて描画を行う。  
/// Transform・UV・カラー・フリップ・アンカーなどの設定が可能。
/// </summary>
class Sprite {
public:
	Sprite() = default;
	~Sprite() = default;

	/// <summary>
	/// スプライトの初期化を行う
	/// </summary>
	/// <param name="textureFilePath">使用するテクスチャファイルパス</param>
	void Initialize(std::string textureFilePath);

	/// <summary>
	/// スプライトの更新処理  
	/// TransformやUVの更新など、描画に必要な行列をセットアップする。
	/// </summary>
	void Update();

	/// <summary>
	/// スプライトの描画処理  
	/// バッファビューやテクスチャSRVを設定して描画を実行する。
	/// </summary>
	void Draw();

private:
	/// <summary>
	/// 頂点バッファリソースを生成する
	/// </summary>
	void CreateVertexResource();

	/// <summary>
	/// インデックスバッファリソースを生成する
	/// </summary>
	void CreateIndexResource();

	/// <summary>
	/// マテリアルリソース（カラーやUV変換情報）を生成する
	/// </summary>
	void CreateMaterialResource();

	/// <summary>
	/// 変換行列リソース（WVP行列など）を生成する
	/// </summary>
	void CreateTransformationResource();

	/// <summary>
	/// スプライトの頂点データを設定する
	/// </summary>
	void SetSpriteData();

	/// <summary>
	/// テクスチャサイズを画像の実寸に合わせて調整する
	/// </summary>
	void AdjustTextureSize();

private:
	/// <summary>
	/// 頂点データ構造体
	/// </summary>
	struct VertexData {
		Vector4 position;  ///< 頂点座標
		Vector2 texcoord;  ///< テクスチャ座標
	};

	/// <summary>
	/// マテリアル構造体
	/// </summary>
	struct Material {
		Vector4 color;          ///< カラー情報(RGBA)
		Matrix4x4 uvTransform;  ///< UV変換行列
	};

	/// <summary>
	/// 変換行列構造体
	/// </summary>
	struct TransformationMatrix {
		Matrix4x4 WVP;    ///< ワールド・ビュー・プロジェクション行列
		Matrix4x4 World;  ///< ワールド行列
	};

private:
	SpriteManager* spriteManager_ = nullptr; ///< スプライト管理クラスへのポインタ

	// GPUリソース
	//Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_ = nullptr;             ///< 頂点バッファ
	//Microsoft::WRL::ComPtr<ID3D12Resource> indexResource_ = nullptr;              ///< インデックスバッファ
	//Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_ = nullptr;           ///< マテリアルバッファ
	//Microsoft::WRL::ComPtr<ID3D12Resource> transformationMatrixResource_ = nullptr;///< 行列バッファ

	uint32_t vertexHandle_ = 0;
	uint32_t indexHandle_ = 0;
	uint32_t materialHandle_ = 0;
	uint32_t transformHandle_ = 0;

	// GPUリソース内データへのポインタ
	VertexData* vertexData_ = nullptr;               ///< 頂点データ
	uint32_t* indexData_ = nullptr;                  ///< インデックスデータ
	Material* materialData_ = nullptr;               ///< マテリアルデータ
	TransformationMatrix* transformationMatrixData_ = nullptr; ///< 行列データ

	// バッファビュー
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{}; ///< 頂点バッファビュー
	D3D12_INDEX_BUFFER_VIEW indexBufferView_{};   ///< インデックスバッファビュー

	// テクスチャSRVハンドル
	D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandleCPU_; ///< テクスチャSRV(CPU側)
	D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU_; ///< テクスチャSRV(GPU側)

	uint32_t textureIndex = 0;            ///< テクスチャ番号
	std::string textureFilePath_;         ///< テクスチャファイルパス

	// フリップ設定
	bool isFlipX_ = false; ///< 左右反転
	bool isFlipY_ = false; ///< 上下反転

private:
	// スプライトの基本情報
	EulerTransform transform_{ {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} };
	Vector2 position_ = { 0.0f, 0.0f }; ///< スプライトの表示位置
	float rotation_ = 0.0f;             ///< スプライトの回転角（ラジアン）
	Vector2 size_ = { 80.0f, 80.0f };   ///< スプライトサイズ

	// UV変換情報
	EulerTransform uvTransform_{ {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} };
	Vector2 uvPosition_ = { 0.0f, 0.0f }; ///< UVオフセット
	float uvRotation_ = 0.0f;             ///< UV回転角
	Vector2 uvSize_ = { 1.0f, 1.0f };     ///< UVスケール

	Vector2 anchorPoint_ = { 0.0f, 0.0f };   ///< アンカーポイント（左上0.0～右下1.0）
	Vector2 textureLeftTop_ = { 0.0f, 0.0f }; ///< テクスチャ切り出し左上座標
	Vector2 textureSize_ = { 100.0f, 100.0f }; ///< テクスチャ切り出しサイズ

public:
	/// <summary>
	/// スプライトの座標を取得する
	/// </summary>
	const Vector2& GetPosition() const { return position_; }

	/// <summary>
	/// スプライトの座標を設定する
	/// </summary>
	void SetPosition(const Vector2& position) { position_ = position; }

	/// <summary>
	/// スプライトの回転角を取得する
	/// </summary>
	float GetRotation() const { return rotation_; }

	/// <summary>
	/// スプライトの回転角を設定する
	/// </summary>
	void SetRotation(float rotation) { rotation_ = rotation; }

	/// <summary>
	/// スプライトの拡縮を取得する
	/// </summary>
	const Vector2& GetSize() const { return size_; }

	/// <summary>
	/// スプライトの拡縮を設定する
	/// </summary>
	void SetSize(const Vector2& size) { size_ = size; }

	/// <summary>
	/// スプライトのカラーを取得する
	/// </summary>
	const Vector4& GetColor() const { return materialData_->color; }

	/// <summary>
	/// スプライトのカラーを設定する
	/// </summary>
	void SetColor(const Vector4& color) { materialData_->color = color; }

	/// <summary>
	/// アンカーポイントを取得する
	/// </summary>
	const Vector2& GetAnchorPoint() const { return anchorPoint_; }

	/// <summary>
	/// アンカーポイントを設定する
	/// </summary>
	void SetAnchorPoint(const Vector2& anchorPoint) { anchorPoint_ = anchorPoint; }

	/// <summary>
	/// 左右反転状態を取得する
	/// </summary>
	const bool& GetIsFlipX() const { return isFlipX_; }

	/// <summary>
	/// 左右反転を設定する
	/// </summary>
	void SetIsFlipX(bool isFlipX) { isFlipX_ = isFlipX; }

	/// <summary>
	/// 上下反転状態を取得する
	/// </summary>
	const bool& GetIsFlipY() const { return isFlipY_; }

	/// <summary>
	/// 上下反転を設定する
	/// </summary>
	void SetIsFlipY(bool isFlipY) { isFlipY_ = isFlipY; }

	/// <summary>
	/// テクスチャの左上座標を取得する
	/// </summary>
	const Vector2& GetTextureLeftTop() const { return textureLeftTop_; }

	/// <summary>
	/// テクスチャの左上座標を設定する
	/// </summary>
	void SetTextureLeftTop(Vector2 textureLeftTop) { textureLeftTop_ = textureLeftTop; }

	/// <summary>
	/// テクスチャの切り出しサイズを取得する
	/// </summary>
	const Vector2& GetTextureSize() const { return textureSize_; }

	/// <summary>
	/// テクスチャの切り出しサイズを設定する
	/// </summary>
	void SetTextureSize(Vector2 textureSize) { textureSize_ = textureSize; }

	/// <summary>
	/// UV平行移動を取得する
	/// </summary>
	const Vector2& GetUVPosition() const { return uvPosition_; }

	/// <summary>
	/// UV平行移動を設定する
	/// </summary>
	void SetUVPosition(const Vector2& position) { uvPosition_ = position; }

	/// <summary>
	/// UV回転を取得する
	/// </summary>
	float GetUVRotation() const { return uvRotation_; }

	/// <summary>
	/// UV回転を設定する
	/// </summary>
	void SetUVRotation(float rotation) { uvRotation_ = rotation; }

	/// <summary>
	/// UV拡縮を取得する
	/// </summary>
	const Vector2& GetUVSize() const { return uvSize_; }

	/// <summary>
	/// UV拡縮を設定する
	/// </summary>
	void SetUVSize(const Vector2& size) { uvSize_ = size; }
};
