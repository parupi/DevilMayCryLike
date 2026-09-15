#pragma once
#include "World3D/Object/Object3d.h"
#include "BaseState/EnemyStateBase.h"
#include "GameObject/Effect/HitStop.h"
#include "GameObject/Character/CharacterStructs.h"
#include "GameObject/Character/Combat/KnockbackComponent.h"
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
	/// 接地したときに地面へわずかに沈める量[m]。
	/// コライダーを地面と重ねたままにするための遊びで、これが無いと押し出した次のフレームに
	/// 接触が切れて OnCollisionStay が来なくなり、接地判定が明滅して落下と着地を繰り返す。
	/// 見た目（モデルの足元）はこのぶん持ち上げて打ち消す（ApplyModelGroundOffset）
	/// </summary>
	static constexpr float kGroundSink = 0.1f;

	/// <summary>ノックバック・落下に使う重力[m/s^2]</summary>
	static constexpr float kGravity = -9.8f;

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
	/// とどめの一撃で吹き飛ばす初速を与える。
	/// 死亡演出中はステートの更新が止まるため、KnockBack ステートを経由せずここで直接与える。
	/// 空中コンボのように吹き飛ばしが 0 の攻撃でも「少し吹っ飛ぶ」よう下限を設けている。
	/// </summary>
	void ApplyDeathLaunch(const Vector3& direction, const KnockbackData& knockback);

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

	// ======================
	// ノックバック（仕様書 §6, §9）
	// ======================

	/// <summary>
	/// ノックバックの速度を持つ部品。移動の速度（velocity_）とは分けて持ち、
	/// Enemy::Update が両方を足して位置へ反映する。
	/// </summary>
	KnockbackComponent& GetKnockback() { return knockback_; }
	const KnockbackComponent& GetKnockback() const { return knockback_; }

	/// <summary>
	/// この敵のノックバック耐性（仕様書 §9）。
	/// 派生クラスがコンストラクタか Initialize で SetKnockbackResistance() を呼んで設定する。
	/// 状況で変えたい場合（アーマー中だけ固くする等）はオーバーライドしてよい。
	/// </summary>
	virtual const KnockbackResistance& GetKnockbackResistance() const { return knockbackResistance_; }
	void SetKnockbackResistance(const KnockbackResistance& resistance) { knockbackResistance_ = resistance; }

	/// <summary>
	/// 解決済みのヒット情報を受けてノックバックを始める（仕様書 §20 ⑤⑥）。
	/// 被弾リアクションのステートへ入るかは causesReaction で決める。
	/// 演出（ヒットストップ・VFX）は呼び出し側の担当。
	/// </summary>
	void ApplyKnockback(const DamageInfo& info, bool causesReaction);

	/// <summary>そのステートが登録されているか（KnockBack を持たない敵があるので確認に使う）</summary>
	bool HasState(const std::string& stateName) const { return states_.find(stateName) != states_.end(); }

	/// <summary>
	/// スタイルスコアの敵補正倍率。難しい敵ほど高くする（雑魚1.0 / ボス2.0 など）。
	/// 攻撃ヒット・撃破時の加点に掛かる。
	/// </summary>
	virtual float GetStyleMultiplier() const { return 1.0f; }

	/// <summary>斬られたときの手応えの材質。プレイヤーの剣のヒット演出が火花・破片の種類を変える</summary>
	enum class HitMaterial {
		Flesh, // 生身（赤い霧と小さな飛沫。控えめ）
		Bone,  // 骨（白い欠片と粉）
		Armor, // 鎧・硬い鱗（金属の火花）
		Wood,  // 木（木片）
	};
	virtual HitMaterial GetHitMaterial() const { return HitMaterial::Flesh; }

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
	/// 死亡演出（吹き飛び〜ディゾルブ）に入っているか。
	/// 演出が終わるまで isAlive_ は落ちないので、「もう倒した敵」を除きたい側
	/// （ロックオン対象・攻撃の当たり先など）はこちらを見る。
	/// </summary>
	bool IsDying() const {
		return appearanceFx_ && (appearanceFx_->IsDying() || appearanceFx_->IsDeathFinished());
	}

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
	/// 体が向いている水平方向（正規化済み）。敵の前方向は **ローカル -Z**。
	/// 向きが取れないとき（真上を向いている等）は +Z を返す。
	/// </summary>
	Vector3 GetForward();

	/// <summary>
	/// 足元のワールド座標。X/Z は本体、Y は **コライダーの底**（＝立っている地面の高さ）。
	/// 攻撃予兆のように地面へ置く演出の基準に使う。
	/// オブジェクト原点はコライダーの中心なので、原点をそのまま使うと宙に浮く
	/// </summary>
	Vector3 GetFootPosition();

	/// <summary>
	/// 体の中心のワールド座標（＝コライダーの中心）。ロックオンの狙う位置に使う。
	/// オブジェクト原点はモデルとステージ側のコライダーの置き方で高さが変わり、
	/// ボス(Dragon)はコライダーを上へずらしてあるので原点が足元より下にある
	/// </summary>
	Vector3 GetBodyCenter();

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

	/// <summary>
	/// 体のモデルに掛ける固定の縦補正。派生クラスがコンストラクタ（レンダラー登録後）で呼ぶ。
	///
	/// 既定では **モデルの原点が足元にある** ものとして、足がコライダーの底＝立っている地面に
	/// 来るように自動で下げる（ApplyModelGroundOffset）。原点が腰や胴にあるモデルを使うときだけ、
	/// そのズレをここで足す。値は **オブジェクトのスケール1 のときのワールド単位**（上が正）。
	/// </summary>
	void SetModelGroundOffset(float offset) { modelGroundOffset_ = offset; }

	/// <summary>
	/// 死亡モーションの最後のポーズが地面から浮いているモデル用。倒れるのに合わせてモデルをこの量だけ沈める。
	/// 単位は SetModelGroundOffset と同じ（オブジェクトのスケール1のときのワールド単位、正で下へ）
	/// </summary>
	void SetDeathModelSink(float amount) { deathModelSink_ = amount; }

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
	void EndAttackAnimation() { attackFitSeconds_ = 0.0f; attackSpeedOverride_ = 0.0f; }

	/// <summary>
	/// 攻撃クリップの再生速度を直接指定する。
	/// 「予備動作だけを引き伸ばす（溜めはゆっくり・振り抜きは等速）」のように、
	/// 攻撃の途中で速度を切り替えたいときに使う。
	/// UpdateAnimation が毎フレーム速度を書き直すので、AnimationPlayer::SetSpeed を
	/// 直接呼んでも1フレームで打ち消される。**必ずこちらを通すこと**。
	/// </summary>
	void SetAttackAnimationSpeed(float speed) { attackSpeedOverride_ = speed; }
	/// <summary>速度の指定を解除して等速（または攻撃の尺合わせ）に戻す</summary>
	void ClearAttackAnimationSpeed() { attackSpeedOverride_ = 0.0f; }

	// ======================
	// 向き
	// ======================

	/// <summary>
	/// 体の向きを forward（水平方向）へ揃えて固定する。固定している間はプレイヤーへ向き直らない。
	/// 攻撃の振り始めに EnemyAttackAim が呼び、予兆を出した向きのまま攻撃させる
	/// （向き直りを残すと、予兆が消えた後に横へ動いたプレイヤーの方へ攻撃が曲がる）。
	/// **固定したら攻撃の終わり・中断で必ず UnlockFacing() すること**（忘れると以降ずっと振り向かない）
	/// </summary>
	void LockFacing(const Vector3& forward);
	void UnlockFacing() { facingLocked_ = false; }
	bool IsFacingLocked() const { return facingLocked_; }

	/// <summary>
	/// プレイヤーへ向き直る速さの上限[度/秒]。0 以下なら毎フレームぴったり向く（既定）。
	/// 大きな敵に重さを出したいとき、派生クラスのコンストラクタで設定する
	/// </summary>
	void SetFaceTurnSpeed(float degreesPerSecond) { faceTurnSpeed_ = degreesPerSecond; }
	/// <summary>
	/// 向き直りの速さを一時的に上書きする（攻撃の溜めの間だけ遅くする等）。0 以下なら上書きしない。
	/// 上書きしたら終わりで必ず ClearFaceTurnSpeedOverride() すること
	/// </summary>
	void SetFaceTurnSpeedOverride(float degreesPerSecond) { faceTurnSpeedOverride_ = degreesPerSecond; }
	void ClearFaceTurnSpeedOverride() { faceTurnSpeedOverride_ = 0.0f; }

	/// <summary>体のアニメーション再生窓口。静的モデルを使っている間は nullptr が返る</summary>
	AnimationPlayer* GetAnimationPlayer();

protected:
	/// <summary>
	/// 死亡演出（ディゾルブアウト）が終わった直後に一度だけ呼ばれる。
	/// 武器など本体以外の後始末を派生クラスで行う。
	/// </summary>
	virtual void OnDeathEffectFinished() {}

	/// <summary>
	/// ノックバック中に壁（移動範囲の境界）へぶつかった瞬間に1度だけ呼ばれる（仕様書 §10）。
	/// 既定では体を光らせるだけ。派生クラスで火花などを足せる。
	/// </summary>
	virtual void OnKnockbackWallHit();

	std::unordered_map<std::string, std::unique_ptr<EnemyStateBase>> states_;
	EnemyStateBase* currentState_ = nullptr;

	// 出現・死亡演出（粒子 + ディゾルブ）
	std::unique_ptr<EnemyAppearanceEffect> appearanceFx_;

	// 敵に追従するポイントライト（被弾時にフラッシュ）
	std::unique_ptr<CharacterLight> characterLight_;

	LockOnTarget lockOnTarget_;

	Player* player_ = nullptr;

	// 移動（意思決定のステートが書く）速度。ノックバックはここには混ぜない
	Vector3 velocity_{};
	Vector3 acceleration_{0.0f, 0.0f, 0.0f};

	// ノックバックの速度（仕様書 §6 の knockbackVelocity）。
	// 被弾リアクションのステートに入らない敵（ボス）でも押されるよう、ステートではなく本体が持つ
	KnockbackComponent knockback_;
	KnockbackResistance knockbackResistance_{};
	// 壁ヒット演出を1回のノックバックにつき1度だけ出すためのガード
	bool knockbackWallHit_ = false;

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

	// 体のモデルの足元をコライダーの底（＝立っている地面）に合わせてレンダラーへ書き込む。
	// コライダーの大きさはステージ側のデータなので、毎フレーム引き直して追従させる
	// （エディタでコライダーを変えてもその場で合う）。
	void ApplyModelGroundOffset();

	Quaternion modelRotationOffset_ = Identity();   // モデル固有の向き補正（差し替えても変わらない）
	Quaternion modelReactionRotation_ = Identity(); // 被弾リアクション（毎フレーム変わる）
	float modelGroundOffset_ = 0.0f;                // モデル固有の縦補正（原点が足元でないモデル用）
	float deathModelSink_ = 0.0f;                   // 死亡モーション中に沈める量（SetDeathModelSink）
	float deathSinkTimer_ = 0.0f;                   // 死亡演出に入ってからの経過時間

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
	// 死亡クリップの再生速度の下限。短いクリップを演出の尺いっぱいに引き伸ばすと
	// 止まって見えるので、遅くする側だけ止める（早く終わった分は最後のポーズで倒れたまま）
	static constexpr float kDeathClipSpeedMin = 0.6f;
	// とどめの吹き飛びの下限[m/s]。攻撃側の吹き飛ばしが 0 でも倒れたことが分かるようにする
	static constexpr float kDeathLaunchMinSpeed = 5.0f;
	static constexpr float kDeathLaunchMinUpSpeed = 3.5f;
	std::unordered_map<std::string, StateClip> stateClips_;
	StateClip spawnClip_;
	StateClip deathClip_;
	// 現在のステート名。currentState_ はポインタなので名前は ChangeState で控えておく
	std::string currentStateName_;
	// >0 のとき、攻撃クリップをこの秒数に収まる速度で再生する
	float attackFitSeconds_ = 0.0f;
	// >0 のとき、attackFitSeconds_ より優先してこの速度で攻撃クリップを流す。
	// 予備動作の引き伸ばしのように、ステート側が速度を明示する場合に使う
	float attackSpeedOverride_ = 0.0f;
	// 攻撃の振り始めから終わりまで true。プレイヤーへ向き直らず、予兆を出した向きのまま攻撃する
	bool facingLocked_ = false;
	// 向き直りの速さの上限[度/秒]（0 以下なら即座に向く）と、その一時的な上書き
	float faceTurnSpeed_ = 0.0f;
	float faceTurnSpeedOverride_ = 0.0f;
	// 最後に向けた向き（ローカル +Z が向く水平方向＝プレイヤーと反対側）。
	// ゼロのうちは向きを覚えていないので、次の向き直りは一気に向く
	Vector3 faceDir_{};

	// ローカル +Z を away（水平方向）へ向ける。速さの上限があれば、今の向きから少しずつ回す
	void TurnToward(const Vector3& away, float deltaTime);
	// 次の UpdateAnimation で攻撃クリップを頭から出し直すか（連続攻撃で振り直すため）
	bool attackAnimRestart_ = false;

	// 現在位置を移動範囲(XZ)内に押し戻す。範囲外へ向かう速度も殺して張り付きを防ぐ。
	// 位置を動かした直後（速度の積分後・ノックバックの慣性適用後）に呼ぶこと。
	void ClampToMovementBounds();

	// Groundコライダーとのめり込みを解消する（OnCollisionEnter/Stay共通処理）
	void ResolveGroundCollision(BaseCollider* other);

	// 真下の地面まで即座に降ろす（Spawn時用）。
	// 空中に配置された敵が出現後に落下してくるのを防ぐ。地面が見つからなければ元の位置のまま。
	void SnapToGround();

	// 自分に付いているコライダーの判定をまとめて切り替える（出現前は切っておく）
	void SetCollidersActive(bool active);
};
