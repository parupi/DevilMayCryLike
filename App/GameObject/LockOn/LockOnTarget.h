#pragma once
#include <Math/Vector3.h>

class LockOnSystem;
class Object3d;

class LockOnTarget
{
public:
	// 初期化
	void Initialize(LockOnSystem* system, Object3d* owner);
	// 終了
	void Finalize();

	/// <summary>
	/// システム側から「自分が破棄される」と伝えられたときに呼ばれる。
	/// LockOnSystem のデストラクタ専用。以降 Finalize() は何もしない
	/// （＝解放済みの LockOnSystem を触りに行かない）
	/// </summary>
	void DetachSystem() { system_ = nullptr; }

	/// <summary>
	/// 狙う位置（レティクル・視野判定・ターゲット選び）。敵なら体の中心（コライダーの中心）。
	/// オブジェクト原点はモデルごとに高さがまちまちで、ボス(Dragon)は足元より下にあるため使わない
	/// </summary>
	Vector3 GetWorldPosition() const;

	/// <summary>
	/// 足元の位置（カメラの構図用）。敵ならコライダーの底。
	/// カメラはここへ自分の lookHeight を足して使うので、体の中心を渡すと大きい敵ほど注視点が上がりすぎる
	/// </summary>
	Vector3 GetFootPosition() const;

	bool IsLockable() const;

	// 対象の残りHP割合（0〜1）。敵でなければ1を返す（レティクルのHP表示用）
	float GetHpRatio() const;

	// 対象がノックバック無効（スーパーアーマー）中かどうか。敵でなければfalse（レティクルの色変化用）
	bool IsKnockbackImmune() const;

private:
	LockOnSystem* system_ = nullptr;
	Object3d* owner_ = nullptr;
};

