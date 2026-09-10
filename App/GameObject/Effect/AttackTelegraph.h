#pragma once
#include <vector>
#include <cstdint>
#include <d3d12.h>
#include "Math/Vector2.h"
#include "Math/Vector3.h"
#include "Math/Vector4.h"
#include "Math/Matrix4x4.h"
#include "Graphics/Resource/ResourceManager.h"

/// <summary>
/// 予兆マーカーの形。**AttackMarker.PS.hlsl の kShape* と値を合わせること**
/// </summary>
enum class TelegraphShape {
	None = -1,  // 予兆を出さない
	Circle = 0, // 円。中心から外へ塗られる（自分の周囲を薙ぐ攻撃）
	Fan = 1,    // 扇。敵の正面へ開く（振り下ろし・噛みつき）
	Rect = 2,   // 矩形。敵の正面へ伸びる帯（突進・ブレス）
};

/// <summary>
/// 攻撃ごとに書く予兆の形。
/// 大きさは判定（halfExtents）と同じく **オブジェクトのスケール1 のときのワールド単位** で書く。
/// ステージで敵を大きく置いても予兆が置いていかれないよう、
/// 攻撃コンポーネントが BeginAttack で ApplyScale() を掛けてから使う。
/// </summary>
struct AttackTelegraphParams {
	TelegraphShape shape = TelegraphShape::None;

	float radius = 3.0f;        // Circle / Fan の半径
	float halfAngleDeg = 55.0f; // Fan の半角[度]。正面を0度として左右にこの角度ぶん開く
	float halfWidth = 1.0f;     // Rect の半幅
	float length = 6.0f;        // Rect が正面へ伸びる長さ
	float forwardOffset = 0.0f; // 図形を正面方向へずらす量（足元より前に置きたいとき）

	/// <summary>配置スケールを掛ける。lateral=横方向、forward=前後方向の倍率</summary>
	void ApplyScale(float lateral, float forward);
};

/// <summary>
/// 敵の攻撃予兆を地面に赤く描くシステム（シングルトン）。
///
/// FF14 の範囲攻撃予兆と同じ読み方をする:
///   - 外枠  … 攻撃が当たる範囲そのもの
///   - 内側の塗り … 発生までの進み具合。外枠まで塗りきった瞬間に判定が出る
///
/// 使い方は「予備動作の間だけ毎フレーム Submit()、判定が出たら呼ぶのをやめる」。
/// 呼ばれなくなったマーカーは自動で閃光を出して消えるので、
/// 攻撃側は消し忘れを気にしなくてよい（中断したときだけ Cancel() で即座に消す）。
/// </summary>
class AttackTelegraph {
public:
	/// 同時に出せるマーカーの数。溢れた分は無視する
	static constexpr uint32_t kMaxMarkers = 24;
	/// <summary>
	/// 地面から浮かせる高さ[m]。
	/// 基準にしている「コライダーの底」は Enemy::ResolveGroundCollision が
	/// 0.1m 沈めた位置なので、実際の地面からの余裕はここから 0.1m 引いた分になる。
	/// 浅い角度で床を見たときに床とZファイティングを起こさないだけの高さが要る
	/// （足りないと、遠い側が虫食いのようにチラつく）
	/// </summary>
	static constexpr float kGroundLift = 0.28f;

	static AttackTelegraph& GetInstance();

	/// <summary>
	/// 予備動作の間、毎フレーム呼ぶ。ownerKey は攻撃コンポーネントのポインタなど、
	/// その攻撃を一意に指すもの。同じキーで呼び続けると同じマーカーが更新される。
	/// </summary>
	/// <param name="groundPos">図形の基準点（敵の足元。Y は地面の高さ）</param>
	/// <param name="forward">敵の正面（水平成分。正規化していなくてよい）</param>
	/// <param name="progress">発生までの進み具合 0→1。1になった瞬間に判定が出る</param>
	void Submit(const void* ownerKey, const AttackTelegraphParams& params,
		const Vector3& groundPos, const Vector3& forward, float progress);

	/// <summary>攻撃が中断されたときに呼ぶ。閃光を出さずに即座に消える</summary>
	void Cancel(const void* ownerKey);

	/// <summary>
	/// マーカーの寿命を進める。**敵の更新より前に呼ぶこと**
	/// （前フレームに Submit されなかったマーカーを「判定が出た」とみなして畳む）。
	/// </summary>
	void Update(float deltaTime);

	/// <summary>描画。シーンの Draw から1回呼ぶ</summary>
	void Draw();

	/// <summary>シーンをまたぐときに残骸を消す</summary>
	void Clear();

	void SetEnabled(bool enabled) { enabled_ = enabled; }
	bool IsEnabled() const { return enabled_; }

private:
	AttackTelegraph() = default;

	// AttackMarker.VS.hlsl の InputLayout と一致させること
	struct MarkerVertex {
		Vector3 position; // POSITION : ワールド座標
		Vector2 local;    // TEXCOORD0: 図形ローカル[m]（x=右 / y=前）
		Vector4 param;    // TEXCOORD1: x=形状 y=進み具合 z=扇の半角[rad] w=不透明度
		Vector4 extent;   // TEXCOORD2: x=横の広さ[m] y=奥行きの広さ[m] z=閃光 w=枠の太さ[m]
		Vector4 color;    // COLOR    : rgb=色
	};

	// HLSL の cbuffer MarkerCB (b0) と同じレイアウト
	struct MarkerConstantData {
		Matrix4x4 viewProj;
	};

	struct Marker {
		const void* key = nullptr;
		TelegraphShape shape = TelegraphShape::Circle;
		Vector3 center{};   // 図形の中心（ワールド）
		Vector3 right{};    // 図形の +x 方向（水平・単位ベクトル）
		Vector3 forward{};  // 図形の +y 方向（水平・単位ベクトル）
		float extentX = 1.0f;
		float extentY = 1.0f;
		float halfAngleRad = 1.0f;
		float progress = 0.0f;
		float appear = 0.0f;       // 出現フェード 0→1
		float resolveTimer = -1.0f; // 0以上なら「判定が出た後」。閃光を出しながら消える
		bool submitted = false;     // このフレームに Submit されたか
	};

	void EnsureResources();
	Marker* FindMarker(const void* ownerKey);
	// 1枚ぶんの四角形（6頂点）を書き込む
	void WriteMarker(const Marker& marker, MarkerVertex* dst) const;

	std::vector<Marker> markers_;
	bool enabled_ = true;

	// 動的頂点バッファ（Upload ヒープ。毎フレーム CPU から書き込む）
	BufferHandle vbHandle_ = kInvalidBufferHandle;
	MarkerVertex* mappedVB_ = nullptr;
	D3D12_VERTEX_BUFFER_VIEW vbView_{};

	// 定数バッファ（viewProj）
	BufferHandle cbHandle_ = kInvalidBufferHandle;
	MarkerConstantData* mappedCB_ = nullptr;

	uint32_t vertexCount_ = 0;
};
