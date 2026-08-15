#pragma once
#include "World3D/Object/Object3d.h"
#include "BaseState/EnemyStateBase.h"
#include "GameObject/Effect/HitStop.h"
#include "GameObject/Character/CharacterStructs.h"
#include "GameObject/Character/MovementBounds.h"
#include "GameObject/Character/Enemy/Effect/EnemyAppearanceEffect.h"
#include "GameObject/Effect/CharacterLight.h"
#include "GameObject/Effect/HitFlashComponent.h"
#include <GameObject/LockOn/LockOnTarget.h>

class Player;
class AnimationPlayer;

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
	/// 既定では「死亡抑制フラグが立っていなければ死ねる」。
	/// チュートリアル用の敵などが条件を足すときはオーバーライドし、
	/// このクラスの実装（＝抑制フラグ）も必ず AND で残すこと。
	/// </summary>
	virtual bool CanDie() const { return !deathSuppressed_; }

	/// <summary>
	/// ノックバック無効（スーパーアーマー）中かどうか。
	/// 通常の敵は常に false。ボスなどがオーバーライドする。
	/// ロックオンレティクルの色変化など、プレイヤーへの状態表示に使う。
	/// </summary>
	virtual bool IsKnockbackImmune() const { return false; }

	/// <summary>
	/// スタイルスコアの敵補正倍率。難しい敵ほど高くする（雑魚1.0 / ボス2.0 など）。
	/// 攻撃ヒット・撃破時の加点に掛かる。
	/// </summary>
	virtual float GetStyleMultiplier() const { return 1.0f; }

	/// <summary>
	/// 攻撃行動をしてよいかを返す。
	/// 既定では「攻撃抑制フラグが立っていなければ攻撃できる」。
	/// 意思決定ステート（CombatIdleなど）が攻撃を選ぶ前にこれを確認する。
	/// </summary>
	virtual bool CanAttack() const { return !attackSuppressed_; }

	// ======================
	// 外部からの行動制御（トレーニングルームなどのデバッグ用途）
	// ======================

	/// <summary>true の間、HP が 0 になっても死なない（CanDie が false になる）</summary>
	void SetDeathSuppressed(bool suppress) { deathSuppressed_ = suppress; }
	bool IsDeathSuppressed() const { return deathSuppressed_; }

	/// <summary>true の間、意思決定ステートが攻撃行動を選ばなくなる</summary>
	void SetAttackSuppressed(bool suppress) { attackSuppressed_ = suppress; }
	bool IsAttackSuppressed() const { return attackSuppressed_; }

	/// <summary>
	/// true の間、ステートの更新と自走を止めてその場に立たせる。
	/// 被弾リアクション・出現／死亡演出・重力は従来どおり動く（見た目を確認したいのはそちら）。
	/// </summary>
	void SetActionSuppressed(bool suppress) { actionSuppressed_ = suppress; }
	bool IsActionSuppressed() const { return actionSuppressed_; }

	/// <summary>
	/// 被弾を記録する（表示用）。派生クラスが hp_ を減らす場所で呼ぶ。
	/// HP と違って抑制・回復の影響を受けないので、与ダメージの確認に使える
	/// </summary>
	void RecordDamage(float amount) {
		++damageHitCount_;
		totalDamageTaken_ += amount;
		lastDamageTaken_ = amount;
	}
	uint32_t GetDamageHitCount() const { return damageHitCount_; }
	float GetTotalDamageTaken() const { return totalDamageTaken_; }
	float GetLastDamageTaken() const { return lastDamageTaken_; }

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
	/// 体を一瞬白く光らせる（設計書 §27 Hit Flash）。攻撃がヒットした瞬間に呼ぶ。
	/// </summary>
	void PlayHitFlash() { if (hitFlash_) hitFlash_->Start(); }

	/// <summary>
	/// ヒットフラッシュの対象レンダラーを追加する（武器など本体以外を足すとき用）。
	/// 本体のレンダラーは Enemy::Initialize が自動で登録する。
	/// </summary>
	void AddHitFlashRenderer(BaseRenderer* renderer) { if (hitFlash_) hitFlash_->AddRenderer(renderer); }

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

	float GetMaxHp() const { return maxHp_; }

	/// <summary>
	/// 今のステート名。currentState_ はポインタなので、ChangeState が控えたこちらを見る。
	/// 挙動を詰めるときの状態表示に使う
	/// </summary>
	const std::string& GetCurrentStateName() const { return currentStateName_; }

	bool IsActive() const { return isActive_; }
	void SetActive(bool flag) { isActive_ = flag; }

	void SetIsAttack(bool flag) { isAttack_ = flag; }

	void SetupLockOn(LockOnSystem* lockOnSystem);

	/// <summary>
	/// 移動できる範囲(水平方向)を制限する。強制戦闘イベントで敵がエリア外へ逃げるのを防ぐ。
	/// Player と同じく水平方向だけを見る（落下を妨げないため Y は制限しない）。
	/// </summary>
	void SetMovementBounds(const MovementBounds& bounds) {
		movementBounds_ = bounds; hasMovementBounds_ = true;
	}

	/// <summary>移動範囲の制限を解除する</summary>
	void ClearMovementBounds() { hasMovementBounds_ = false; }

	// ======================
	// モデルの向き
	// ======================

	/// <summary>
	/// 体のモデルに掛ける固定の向き補正。派生クラスがコンストラクタ（レンダラー登録後）で呼ぶ。
	///
	/// Enemy::Update はオブジェクトのローカル -Z がプレイヤーを向くように回転させる
	/// （武器の構え位置・振り抜き先が -Z 側にあるのはこのため）。
	/// 一方 Blender から出した .obj はモデルの正面が +Z なので、そのままだと後ろ姿で戦うことになる。
	/// そのため正面が +Z のモデルには Y 軸 180 度を渡す。
	/// モデルを差し替えて後ろ向きになったら、まずこの角度を疑うこと。
	/// </summary>
	void SetModelRotationOffset(const Quaternion& offset) {
		modelRotationOffset_ = offset;
		ApplyModelRotation();
	}

	/// <summary>
	/// 被弾リアクション（のけぞりの傾き・吹き飛びの回転）を体のモデルに与える。
	/// 向き補正と合成して適用されるので、ステート側はレンダラーの回転を直接書かないこと。
	/// </summary>
	void SetModelReactionRotation(const Quaternion& reaction) {
		modelReactionRotation_ = reaction;
		ApplyModelRotation();
	}

	/// <summary>被弾リアクションの回転を消して、向き補正だけの状態に戻す。</summary>
	void ClearModelReactionRotation() { SetModelReactionRotation(Identity()); }

	// ======================
	// アニメーション
	// ======================

	/// <summary>
	/// ステート名に対して再生するクリップを登録する。派生クラスが Initialize で並べる。
	/// 未登録のステートに入ったときはクリップを切り替えない（直前のものが続く）ので、
	/// 見た目を変えたくないステートは登録しなくてよい。
	///
	/// impactRatio は攻撃クリップ専用で、「振り切る瞬間がクリップ全体のどこか」を 0〜1 で渡す。
	/// これを使って、武器が斬り抜ける瞬間と体のモーションの山が重なるように再生速度を決める。
	/// 値は gltf のキーフレームから角速度のピークを測って求めた（クリップを差し替えたら測り直す）。
	/// </summary>
	void RegisterStateClip(const std::string& stateName, const std::string& clipName,
		bool loop = true, float impactRatio = 0.5f);

	/// <summary>
	/// 出現演出中に流すクリップ。空なら切り替えない。
	/// loop=false なら演出の長さちょうどで1回流れるよう再生速度を合わせる（出現専用モーション向け）。
	/// 出現専用のクリップが無いモデルは待機クリップを loop=true で渡す
	/// </summary>
	void SetSpawnClip(const std::string& clipName, bool loop = false) { spawnClip_ = { clipName, loop }; }
	/// <summary>死亡演出中に流すクリップ。空なら切り替えない。演出の長さに合わせて1回流す</summary>
	void SetDeathClip(const std::string& clipName) { deathClip_ = { clipName, false }; }

	/// <summary>
	/// 攻撃モーションを武器の振りに合わせて再生する。
	/// 武器の動き（CatmullRom）は今までどおりで、体のクリップの方を伸縮させて合わせる。
	/// EnemyMeleeAttackComponent が「武器が斬り抜ける瞬間までの秒数」を渡して呼ぶ。
	/// </summary>
	void BeginAttackAnimation(float weaponImpactSeconds);
	/// <summary>攻撃が終わって等速に戻す</summary>
	void EndAttackAnimation() { attackFitSeconds_ = 0.0f; }

	/// <summary>体のアニメーション再生窓口。静的モデルを使っている間は nullptr が返る</summary>
	AnimationPlayer* GetAnimationPlayer();

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

	// 撃破スコアを二重加算しないためのガード（OnDeathは演出中に複数回呼ばれ得る）
	bool killScored_ = false;

	DamageInfo pendingDamageInfo_;

	std::unique_ptr<HitStop> hitStop_;

	// 被弾時に体を白く光らせるコンポーネント（EmissiveTintを使う）
	std::unique_ptr<HitFlashComponent> hitFlash_;

	bool hasMovementBounds_ = false;
	MovementBounds movementBounds_{};

	// 外部からの行動制御（トレーニングルーム用。通常のプレイでは全部 false のまま）
	bool deathSuppressed_ = false;
	bool attackSuppressed_ = false;
	bool actionSuppressed_ = false;

	// 被弾の記録（表示用）
	uint32_t damageHitCount_ = 0;
	float totalDamageTaken_ = 0.0f;
	float lastDamageTaken_ = 0.0f;

private:
	// 体のモデルの回転を「向き補正 → 被弾リアクション」の順で組み立ててレンダラーへ書き込む。
	// 行ベクトル規約なのでクォータニオンの積は「後に掛けるもの * 先に掛けるもの」になる。
	void ApplyModelRotation();

	Quaternion modelRotationOffset_ = Identity();   // モデル固有の向き補正（差し替えても変わらない）
	Quaternion modelReactionRotation_ = Identity(); // 被弾リアクション（毎フレーム変わる）

	// ステートと演出フェーズから再生クリップを決めて流す。毎フレーム呼ぶ
	void UpdateAnimation();

	struct StateClip {
		std::string clip;
		bool loop = true;
		float impactRatio = 0.5f; // 攻撃クリップのみ使用。振り切る瞬間の位置（0〜1）
	};

	// 攻撃時の再生速度の上下限。極端に短い攻撃で倍率が跳ね上がって残像になるのを防ぐ
	static constexpr float kAttackSpeedMin = 0.5f;
	static constexpr float kAttackSpeedMax = 3.0f;
	std::unordered_map<std::string, StateClip> stateClips_;
	StateClip spawnClip_;
	StateClip deathClip_;
	// 現在のステート名。currentState_ はポインタなので名前は ChangeState で控えておく
	std::string currentStateName_;
	// >0 のとき、攻撃クリップをこの秒数に収まる速度で再生する
	float attackFitSeconds_ = 0.0f;
	// 次の UpdateAnimation で攻撃クリップを頭から出し直すか（連続攻撃で振り直すため）
	bool attackAnimRestart_ = false;

	// 現在位置を移動範囲(XZ)内に押し戻す。範囲外へ向かう速度も殺して張り付きを防ぐ。
	// 位置を動かした直後（速度の積分後・ノックバックの慣性適用後）に呼ぶこと。
	void ClampToMovementBounds();

	// Groundコライダーとのめり込みを解消する（OnCollisionEnter/Stay共通処理）
	// resetVelocity: 接地面に押し出した際にvelocity_.yを0にリセットするか
	void ResolveGroundCollision(BaseCollider* other, bool resetVelocity);

	// 真下の地面まで即座に降ろす（Spawn時用）。
	// 空中に配置された敵が出現後に落下してくるのを防ぐ。地面が見つからなければ元の位置のまま。
	void SnapToGround();
};
