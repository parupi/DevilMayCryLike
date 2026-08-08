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

	const Vector3& GetWorldPosition() const;
	bool IsLockable() const;

	// 対象の残りHP割合（0〜1）。敵でなければ1を返す（レティクルのHP表示用）
	float GetHpRatio() const;

	// 対象がノックバック無効（スーパーアーマー）中かどうか。敵でなければfalse（レティクルの色変化用）
	bool IsKnockbackImmune() const;

private:
	LockOnSystem* system_ = nullptr;
	Object3d* owner_ = nullptr;
};

