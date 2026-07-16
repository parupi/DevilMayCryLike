#pragma once
#include "World3D/Object/Object3d.h"
#include "BaseState/EnemyStateBase.h"
#include "GameObject/Effect/HitStop.h"
#include "GameObject/Character/CharacterStructs.h"
#include "GameObject/Character/Enemy/Effect/EnemyAppearanceEffect.h"
#include "GameObject/Effect/CharacterLight.h"
#include <GameObject/LockOn/LockOnTarget.h>

class Player;

/// <summary>
/// 敵キャラクターの基底クラス。
/// ステートマシン・物理演算・地形衝突・接地判定・死亡管理を担う。
/// 武器・エフェクト・被弾ロジックなどキャラクター固有の処理は派生クラスで実装する。
/// </summary>
class Enemy : public Object3d {
public:
	Enemy(std::string objectName);
	virtual ~Enemy() override;

	/// <summary>
	/// 敵の初期化処理  
	/// 各種ステート・エフェクト・物理パラメータを設定する。
	/// </summary>
	virtual void Initialize() override;

	/// <summary>
	/// 敵の更新処理  
	/// 現在の状態（ステート）に応じた行動・移動・攻撃を行う。
	/// </summary>
	virtual void Update(float deltaTime) override;

	/// <summary>
	/// 敵の描画処理  
	/// モデルや外見をレンダリングする。
	/// </summary>
	virtual void Draw() override;

	/// <summary>
	/// 敵のエフェクト描画処理  
	/// パーティクルやダメージエフェクトなどを描画する。
	/// </summary>
	virtual void DrawEffect();

	void Spawn();

#ifdef _DEBUG
	/// <summary>
	/// デバッグ用GUI描画処理  
	/// 敵の内部状態（HP、速度、状態名など）を可視化する。
	/// </summary>
	virtual void DebugGui() override;
#endif // _DEBUG

	// ======================
	// 衝突処理
	// ======================

	/// <summary>
	/// 衝突開始時の処理  
	/// プレイヤー攻撃や地形との最初の接触時に呼ばれる。
	/// </summary>
	virtual void OnCollisionEnter([[maybe_unused]] BaseCollider* other) override;

	/// <summary>
	/// 衝突中の処理  
	/// 接触中のフレームごとに呼ばれる。
	/// </summary>
	virtual void OnCollisionStay([[maybe_unused]] BaseCollider* other) override;

	/// <summary>
	/// 衝突終了時の処理  
	/// 接触が解除された際に呼ばれる。
	/// </summary>
	virtual void OnCollisionExit([[maybe_unused]] BaseCollider* other) override;

	// ======================
	// ステート・挙動制御
	// ======================

	/// <summary>
	/// ステートを切り替える。KnockBack ステートに情報を渡す場合は
	/// あらかじめ SetPendingDamageInfo() を呼ぶこと。
	/// </summary>
	void ChangeState(const std::string& stateName);

	/// <summary>
	/// HP が 0 になったときに呼ぶ。isAlive_ を false にしてコンポーネントを無効化する。
	/// </summary>
	void OnDeath();

	/// <summary>
	/// HP が 0 になったときに実際に死亡してよいかを返す。
	/// 通常は常に true。チュートリアル用の敵などが条件付きで死亡を抑制するためにオーバーライドする。
	/// </summary>
	virtual bool CanDie() const { return true; }

	/// <summary>
	/// ノックバック無効（スーパーアーマー）中かどうか。
	/// 通常の敵は常に false。ボスなどがオーバーライドする。
	/// ロックオンレティクルの色変化など、プレイヤーへの状態表示に使う。
	/// </summary>
	virtual bool IsKnockbackImmune() const { return false; }

	/// <summary>
	/// 攻撃行動をしてよいかを返す。
	/// 通常は常に true。チュートリアル用の敵（練習台）などが攻撃を封じるためにオーバーライドする。
	/// 意思決定ステート（CombatIdleなど）が攻撃を選ぶ前にこれを確認する。
	/// </summary>
	virtual bool CanAttack() const { return true; }

	bool IsAlive() const { return isAlive_; }

	/// <summary>
	/// 出現・死亡演出の再生中かどうか。
	/// 演出中は被弾処理を行わない（派生クラスの OnCollisionEnter でガードする）。
	/// </summary>
	bool IsAppearanceEffectPlaying() const { return appearanceFx_ && appearanceFx_->IsPlaying(); }

	/// <summary>出現・死亡演出を取得する（武器のレンダラー登録などに使う）</summary>
	EnemyAppearanceEffect* GetAppearanceFx() { return appearanceFx_.get(); }

	/// <summary>
	/// 追従ライトをひときわ強く光らせる。攻撃がヒットした瞬間に呼ぶ。
	/// </summary>
	void FlashLight() { if (characterLight_) characterLight_->Flash(); }

	/// <summary>
	/// KnockBack ステートが Enter() で参照するダメージ情報をセットする。
	/// ChangeState("KnockBack") の直前に呼ぶこと。
	/// </summary>
	void SetPendingDamageInfo(const DamageInfo& info) { pendingDamageInfo_ = info; }
	const DamageInfo& GetPendingDamageInfo() const { return pendingDamageInfo_; }

	// ======================
	// Getter / Setter
	// ======================

	/// <summary>
	/// 地面に接地しているかどうかを取得する。
	/// </summary>
	bool GetOnGround() const { return onGround_; }
	void SetOnGround(bool flag) { onGround_ = flag; }

	/// <summary>
	/// プレイヤーのポインタを取得する。
	/// </summary>
	Player* GetPlayer() { return player_; }

	/// <summary>
	/// プレイヤーのポインタを設定する。
	/// </summary>
	void SetPlayer(Player* player) { player_ = player; }

	/// <summary>
	/// 現在の速度ベクトルを取得する。
	/// </summary>
	Vector3 GetVelocity() const { return velocity_; }

	/// <summary>
	/// 現在の速度ベクトルを設定する。
	/// </summary>
	void SetVelocity(const Vector3& velocity) { velocity_ = velocity; }

	/// <summary>
	/// 現在の加速度ベクトルを取得する。
	/// </summary>
	Vector3 GetAcceleration() const { return acceleration_; }

	/// <summary>
	/// 加速度ベクトルを設定する。
	/// </summary>
	void SetAcceleration(const Vector3& acceleration) { acceleration_ = acceleration; }

	/// <summary>
	/// 現在のHPを取得する。
	/// </summary>
	float GetHp() const { return hp_; }

	/// <summary>
	/// HPを設定する。
	/// </summary>
	void SetHp(float hp) { hp_ = hp; }

	/// <summary>
	/// 残りHPの割合（0〜1）を取得する。ロックオンレティクルのHP表示などに使う。
	/// </summary>
	float GetHpRatio() const {
		if (maxHp_ <= 0.0f) return 0.0f;
		float ratio = hp_ / maxHp_;
		return ratio < 0.0f ? 0.0f : (ratio > 1.0f ? 1.0f : ratio);
	}

	bool IsActive() const { return isActive_; }
	void SetActive(bool flag) { isActive_ = flag; }

	void SetIsAttack(bool flag) { isAttack_ = flag; }

	void SetupLockOn(LockOnSystem* lockOnSystem);

protected:
	/// <summary>
	/// 死亡演出（ディゾルブアウト）が終わった直後に一度だけ呼ばれる。
	/// 武器など本体以外の後始末を派生クラスで行う。
	/// </summary>
	virtual void OnDeathEffectFinished() {}

	std::unordered_map<std::string, std::unique_ptr<EnemyStateBase>> states_;
	EnemyStateBase* currentState_ = nullptr;

	// 出現・死亡演出（粒子 + ディゾルブ）
	std::unique_ptr<EnemyAppearanceEffect> appearanceFx_;

	// 敵に追従するポイントライト（被弾時にフラッシュ）
	std::unique_ptr<CharacterLight> characterLight_;

	LockOnTarget lockOnTarget_;

	Player* player_ = nullptr;

	Vector3 velocity_{};
	Vector3 acceleration_{0.0f, 0.0f, 0.0f};

	float hp_ = 3.0f;
	float maxHp_ = 3.0f; // 派生クラスで hp_ を変えるときは一緒に設定する（GetHpRatio用）
	bool onGround_ = false;
	bool isActive_ = true;
	bool isAlive_ = true;

	bool isAttack_ = false;

	DamageInfo pendingDamageInfo_;

	std::unique_ptr<HitStop> hitStop_;

private:
	// Groundコライダーとのめり込みを解消する（OnCollisionEnter/Stay共通処理）
	// resetVelocity: 接地面に押し出した際にvelocity_.yを0にリセットするか
	void ResolveGroundCollision(BaseCollider* other, bool resetVelocity);

	// 真下の地面まで即座に降ろす（Spawn時用）。
	// 空中に配置された敵が出現後に落下してくるのを防ぐ。地面が見つからなければ元の位置のまま。
	void SnapToGround();
};
