#pragma once
#include <Math/MathUtils.h>
#include <Math/Vector4.h>
#include <Math/Vector3.h>
#include <Math/Vector2.h>

enum class FadeType {
	None = 0, // フェードしない
	Alpha = 1, // 透明度で消える
	ScaleShrink = 2, // 寿命末期で急速に縮む
};

struct Particle {
	EulerTransform transform;
	Vector3 velocity;
	Vector4 color;
	float lifeTime;
	float currentTime;
	bool isAlive;
	Vector3 initialScale; // 生成時のスケールを保持
	float shrinkStart; // 縮小開始時間 (0.0f - 1.0f)
	FadeType fadeType = FadeType::Alpha; // デフォルトをAlphaに
	bool isBillboard;

	// ── 方向付き発生の向き ──
	// orientToDirection が真のとき、形状の基準軸を orientDir へ向けて描画する。
	// Billboard が有効な場合はそちらが優先される。
	bool orientToDirection = false;
	Vector3 orientDir{ 0.0f, 0.0f, 1.0f };

	// ── Curve / Gradient 用の基準値 ──
	// 生成時の値を保持しておき、毎フレーム「基準値 × カーブ値」で現在値を作り直す。
	// transform.scale / color を直接減衰させると誤差が蓄積するためこの持ち方にしている。
	Vector3 baseScale;  // 生成時のスケール（sizeCurve の基準）
	Vector4 baseColor;  // 生成時のカラー（colorGradient / alphaCurve の基準）
};
