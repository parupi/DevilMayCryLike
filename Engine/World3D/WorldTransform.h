#pragma once

#include "Math/Matrix4x4.h"
#include "Math/Vector3.h"
#include "Math/Quaternion.h"
#include <d3d12.h>
#include <type_traits>
#include <wrl.h>

class BaseCamera;

struct TransformationMatrix {
	Matrix4x4 WVP;
	Matrix4x4 World;
	Matrix4x4 WorldInverseTranspose;
};

/// <summary>
/// ワールド変換データ
/// </summary>
class WorldTransform {
public:

	WorldTransform() = default;
	~WorldTransform();

	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize();
	/// <summary>
	/// 定数バッファ生成
	/// </summary>
	void CreateConstBuffer();
	/// <summary>
	/// 行列を転送する
	/// </summary>
	void TransferMatrix(BaseCamera* camera);
	/// <summary>
	/// 定数バッファの取得
	/// </summary>
	/// <returns>定数バッファ</returns>
	//const Microsoft::WRL::ComPtr<ID3D12Resource>& GetConstBuffer() const;

	void BindToShader(ID3D12GraphicsCommandList* cmd, int32_t index) const;

#ifdef _DEBUG
	/// デバッグ用の関数
	void DebugGui();
#endif // _DEBUG

	/// <summary>
	/// マップのセット
	/// </summary>
	/// <param name="wvp">WVP行列</param>
	const Matrix4x4& GetMatWorld() { return matWorld_; }

	// アクセッサ	
	Vector3& GetTranslation() { return translation_; }
	Quaternion& GetRotation() { return rotation_; }
	Vector3& GetScale() { return scale_; }
	void SetParent(WorldTransform* parent) { parent_ = parent; }
	WorldTransform* GetParent() { return parent_; }
	void DetachParent() { parent_ = nullptr; }

	/// <summary>
	/// ローカル変換と親の間に差し込む行列。ワールド行列は local * attach * 親 になる。
	/// ボーン追従（BoneAttachment）がジョイントのスケルトン空間行列をここへ入れる。
	/// 既定は単位行列なので、使わない限り従来どおり local * 親
	/// </summary>
	void SetAttachMatrix(const Matrix4x4& matrix) { attachMatrix_ = matrix; hasAttachMatrix_ = true; }
	void ClearAttachMatrix() { hasAttachMatrix_ = false; }
	// ワールド座標を取得
	Vector3 GetWorldPos() const;
	// ワールドスケールを取得
	Vector3 GetWorldScale() const;

	// 前方向を取得
	Vector3 GetForward() const;
	void SetMatWVP(const Matrix4x4& mat) { constMap->WVP = mat; }
private:
	// 定数バッファ
	//Microsoft::WRL::ComPtr<ID3D12Resource> constBuffer_;
	uint32_t bufferHandle_ = 0;
	// マッピング済みアドレス
	TransformationMatrix* constMap = nullptr;

	// ローカルスケール
	Vector3 scale_ = { 1, 1, 1 };
	// X,Y,Z軸回りのローカル回転角
	Quaternion rotation_;
	// ローカル座標
	Vector3 translation_{};
	// ローカル → ワールド変換行列
	Matrix4x4 matWorld_;
	// 親となるワールド変換へのポインタ
	WorldTransform* parent_ = nullptr;
	// ボーン追従などでローカルと親の間に挟む行列
	Matrix4x4 attachMatrix_;
	bool hasAttachMatrix_ = false;
	// ワールド座標を保持しておく
	Vector3 worldPos_{};

	/// <summary>
	/// ワールド行列を組み立てる材料。前フレームとの比較用にまとめて持つ（memcmpで比較するのでPOD）
	/// </summary>
	struct TransformSource {
		Vector3 scale{};
		Quaternion rotation{};
		Vector3 translation{};
		Matrix4x4 attach{};
		Matrix4x4 parentWorld{};
		uint32_t hasAttach = 0;
	};
	TransformSource cachedSource_{};
	Matrix4x4 cachedWorldInverseTranspose_{};
	bool sourceValid_ = false;

	// コピー禁止
	WorldTransform(const WorldTransform&) = delete;
	WorldTransform& operator=(const WorldTransform&) = delete;
};

static_assert(!std::is_copy_assignable_v<WorldTransform>);