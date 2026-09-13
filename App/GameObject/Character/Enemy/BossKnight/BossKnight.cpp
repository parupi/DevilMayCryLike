#include "BossKnight.h"
#include <cmath>
#include "GameObject/Character/Player/Player.h"
#include <World3D/Object/Renderer/RendererManager.h>
#include <World3D/Object/Renderer/ModelRenderer.h>
#include <World3D/Object/Model/ModelManager.h>
#include <World3D/Collider/AABBCollider.h>
#include <World3D/Collider/OBBCollider.h>
#include <World3D/Collider/CollisionManager.h>
#include "GameObject/Character/Combat/CombatHitResolver.h"
#include "GameObject/Character/Enemy/EnemyStateNames.h"
#include "GameObject/Character/Enemy/State/EnemyStateAir.h"
#include "Graphics/Rendering/Particle/ParticleManager.h"
#include "Utility/Logger.h"
#ifdef _DEBUG
#endif

#include "State/BossStateCombatIdle.h"
#include "State/BossStateApproach.h"
#include "State/BossStateSlash.h"
#include "State/BossStateHeavySword.h"
#include "State/BossStateRush.h"
#include "State/BossStateBreath.h"

BossKnight::BossKnight(std::string objectName) : Enemy(objectName) {
	// ModelRenderer は FindModel するだけで読み込みはしないので、ここで読んでおく。
	ModelManager::GetInstance().LoadSkinnedModel(kModelName);
	ModelManager::GetInstance().LoadModel("Sword");
	RendererManager::GetInstance().AddRenderer(std::make_unique<ModelRenderer>(name_, kModelName));
	AddRenderer(RendererManager::GetInstance().FindRender(name_));
	GetRenderer(name_)->GetWorldTransform()->GetScale() = {kModelScale, kModelScale, kModelScale};
	SetModelRotationOffset(EulerDegree({ 0.0f, 180.0f, 0.0f }));

	hp_ = kMaxHp;
	maxHp_ = kMaxHp;

	// ノックバック耐性（仕様書 §9 の「ボス」）。
	// のけぞらない・吹き飛ばない・打ち上がらないのは以前と同じだが、
	// **位置だけは押される**ようにした（resistance のぶんだけ弱まった速度が入る）。
	// 以前は KnockBack ステートを持たないことで無効化していたので、
	// 攻撃が当たってもボスは1ミリも動かず、手応えがヒットストップと発光だけだった
	SetKnockbackResistance(KnockbackResistance{
		/*resistance=*/ kKnockbackResistance,
		/*canStagger=*/ false,
		/*canBlowAway=*/ false,
		/*canLaunch=*/ false,
	});
}

void BossKnight::Initialize() {
	// ── 自コライダーの調整 ──
	// 
	// レベル側の×2補正の撤廃に伴い 0.65f → 1.3f に変更（実効サイズは従来と同じ）
	//auto* col = static_cast<OBBCollider*>(GetCollider(name_));
	//col->GetColliderData().halfExtents *= 1.3f;

	// ── 攻撃判定の生成 ──
	// ドラゴンは剣を持たず、噛みつき・叩きつけ・突進で戦う。
	// 見た目を持たないヒットボックスをジョイントへ追従させ、攻撃ごとに位置と大きさを変える
	CollisionManager::GetInstance().AddCollider(std::make_unique<OBBCollider>(name_ + "Hitbox"));
	CollisionManager::GetInstance().FindCollider(name_ + "Hitbox")->category_ = CollisionCategory::EnemyWeapon;

	auto hitbox = std::make_unique<EnemyHitbox>(name_ + "Hitbox");
	hitbox->AddCollider(CollisionManager::GetInstance().FindCollider(name_ + "Hitbox"));
	hitbox->Initialize();
	hitbox->SetupAttachment(GetRenderer(name_));
	hitbox_ = hitbox.get();
	Object3dManager::GetInstance().AddObject(std::move(hitbox));

	// ── コンポーネント生成 ──
	sensor_ = std::make_unique<EnemySensorComponent>();
	sensor_->SetDetectionRange(25.0f); // 広い感知範囲
	movement_ = std::make_unique<EnemyMovementComponent>();
	boneAttack_ = std::make_unique<EnemyBoneAttackComponent>(hitbox_);

	// ── ステート登録 ──
	// ボスは被弾でのけぞらないので KnockBack ステートは持たない。
	// 「のけぞらない」を決めているのはステートの有無ではなく
	// ノックバック耐性の canStagger=false（コンストラクタ）で、
	// 位置を押す弱いノックバックだけは KnockbackComponent 経由で入る。
	// （とどめの吹き飛びは ApplyDeathLaunch がステートを経由せずに初速を与える）
	states_[EnemyStateName::Air] = std::make_unique<EnemyStateAir>();

	// EnemyStateAir が着地後に "Move" へ遷移する。"Idle" ともども
	// BossStateCombatIdle にマップして動作を引き継ぐ。
	// CombatIdle は3インスタンスに分かれるので、必殺技の解禁記録は battleMemory_ で共有する
	states_[EnemyStateName::Idle] = std::make_unique<BossStateCombatIdle>(sensor_.get(), movement_.get(), kMaxHp, &battleMemory_);
	states_[EnemyStateName::Move] = std::make_unique<BossStateCombatIdle>(sensor_.get(), movement_.get(), kMaxHp, &battleMemory_);

	states_[BossStateName::CombatIdle] = std::make_unique<BossStateCombatIdle>(sensor_.get(), movement_.get(), kMaxHp, &battleMemory_);
	states_[BossStateName::Approach] = std::make_unique<BossStateApproach>(movement_.get());
	states_[BossStateName::Slash] = std::make_unique<BossStateSlash>(boneAttack_.get());
	states_[BossStateName::HeavySword] = std::make_unique<BossStateHeavySword>(boneAttack_.get());
	states_[BossStateName::Rush] = std::make_unique<BossStateRush>(boneAttack_.get());
	states_[BossStateName::Breath] = std::make_unique<BossStateBreath>(boneAttack_.get());

	// ── アニメーションの割り当て（Dragon.gltf の5クリップ）──
	// このモデルには待機が無いので Flying を待機・移動の両方に充てている。
	// 攻撃クリップは EnemyMeleeAttackComponent が武器の振りの長さに合わせて伸縮させる
	RegisterStateClip(EnemyStateName::Idle,            kClipIdle);
	RegisterStateClip(EnemyStateName::Move,            kClipIdle);
	RegisterStateClip(EnemyStateName::Air,             kClipIdle);
	RegisterStateClip(BossStateName::CombatIdle,       kClipIdle);
	RegisterStateClip(BossStateName::Approach,         kClipIdle);
	RegisterStateClip(BossStateName::Slash,            kClipAttack,  false, kAttackImpactRatio);
	RegisterStateClip(BossStateName::HeavySword,       kClipAttack2, false, kAttack2ImpactRatio);
	RegisterStateClip(BossStateName::Rush,             kClipAttack,  false, kAttackImpactRatio);
	// ブレスは叩きつけと同じ大振りのクリップを使い回す。
	// 攻撃の尺(4.2秒)がクリップ(1.67秒)より長いので、EnemyBoneAttackComponent が
	// 引き伸ばして流す（最後のポーズで固まらない）
	RegisterStateClip(BossStateName::Breath,           kClipAttack2, false, kAttack2ImpactRatio);
	// 出現専用のクリップは無いので、ディゾルブ中は Flying をループさせておく
	SetSpawnClip(kClipIdle, true);
	SetDeathClip(kClipDeath);

	// 被弾時のヒットエフェクトは HitEffectSystem の "HitImpact" に一本化したのでここでは持たない。
	// （足元から出る旧エフェクトと違い、武器が実際に当たった位置へ火花とリングが出る）

	// ── パーティクル: チャージエフェクト（予備動作中に収束するリング）──
	ParticleManager::GetInstance().CreateEmitter(name_ + "ChargeEffect");
	chargeEmitter_ = ParticleManager::GetInstance().GetEmitters().at(name_ + "ChargeEffect").get();
	chargeEmitter_->SetParent(GetWorldTransform());
	chargeEmitter_->AddParticle("EnemyChargeRing");

	// ── パーティクル: スーパーアーマー中に体から立ち上る紫のオーラ ──
	// レンダラーのトランスフォーム（スケール1.5込み）を親にして、体のメッシュ表面から発生させる
	ParticleManager::GetInstance().CreateEmitter(name_ + "ArmorAura");
	auraEmitter_ = ParticleManager::GetInstance().GetEmitters().at(name_ + "ArmorAura").get();
	auraEmitter_->SetParent(GetRenderer(name_)->GetWorldTransform());
	auraEmitter_->SetShapeModel(kModelName); // 本体と同じモデルの表面からエミット
	auraEmitter_->AddParticle("BossArmorAura");
	auraEmitter_->GetParticles()[0].count = 50; // 1回のEmitで4粒ずつ出して体の形が読める密度にする

	// ── パーティクル: スーパーアーマー中の被弾で弾かれたことを示す紫の火花 ──
	ParticleManager::GetInstance().CreateEmitter(name_ + "ArmorHitSpark");
	armorHitEmitter_ = ParticleManager::GetInstance().GetEmitters().at(name_ + "ArmorHitSpark").get();
	armorHitEmitter_->SetParent(GetWorldTransform());
	armorHitEmitter_->AddParticle("BossArmorHitSpark");
	armorHitEmitter_->GetParticles()[0].count = 16; // 1ヒットで16粒の火花を散らす

	// ── VFX: 必殺技ブレスの炎 ──
	// パーティクルグループはシーンをまたいで残るので、未登録のときだけ読む。
	// 毎回読むとパーティクルエディタでの調整がボスを1体置くたびに巻き戻る
	if (!ParticleManager::GetInstance().GetEmitters().contains(kBreathVfxName)) {
		if (!ParticleManager::GetInstance().LoadVFX(kBreathVfxName)) {
			Logger::Log("BossKnight: Resource/VFX/BossBreath.vfx.json を読み込めませんでした（炎が出ません）\n");
		}
	}

	Enemy::Initialize();

	// 出現・死亡演出のディゾルブ対象に武器も含める
	if (appearanceFx_) {
		appearanceFx_->AddRenderer(RendererManager::GetInstance().FindRender(name_ + "Weapon"));
	}

	// 初期ステートを CombatIdle に設定（Enemy::Initialize は "Air" にセットする）
	ChangeState(BossStateName::CombatIdle);
}

void BossKnight::Update(float deltaTime) {
	// 死亡演出終了後は武器が後始末済みのため、本体の後始末だけ行う
	if (!IsAlive()) {
		Enemy::Update(deltaTime);
		return;
	}

	// 未出現時は判定を切り、非アクティブ時の共通処理（消灯など）だけ行う
	if (!isActive_) {
		if (hitbox_) hitbox_->Deactivate();
		Enemy::Update(deltaTime);
		return;
	}

	// ボスは被弾で吹き飛ばないので、velocity_.y を横取りするステートが無い。
	// 重力は常にここで掛けてよい
	SetAcceleration({0.0f, GetOnGround() ? 0.0f : -9.8f, 0.0f});

	// 出現・死亡演出中は判定を出さない（EnemyBoneAttackComponent が出していても打ち消す）
	if (IsAppearanceEffectPlaying() && hitbox_) {
		hitbox_->Deactivate();
	}

	// 予備動作中にチャージリングを発射。
	// 溜めが進むほど間隔を詰めて、いつ振ってくるかが見た目から読めるようにする
	if (boneAttack_->IsWindingUp()) {
		const float t = boneAttack_->GetWindupProgress();
		const float interval = kChargeEmitIntervalStart
			+ (kChargeEmitIntervalEnd - kChargeEmitIntervalStart) * t;
		chargeEmitTimer_ += deltaTime;
		if (chargeEmitTimer_ >= interval) {
			chargeEmitter_->Emit();
			chargeEmitTimer_ = 0.0f;
		}
	} else {
		chargeEmitTimer_ = 0.0f;
	}

	// 体の発光と追従ライト（弾かれの紫・攻撃の溜めの橙）をまとめて更新
	UpdateBodyVisual(deltaTime);

	Enemy::Update(deltaTime);

	// ヒットボックスをジョイントへ合わせ直す。
	// **Enemy::Update（＝ポーズ更新）より後**でなければ1フレーム前の姿勢に付いてしまう。
	// 当たり判定は全オブジェクト更新のあとに CollisionManager が見るので、ここで間に合う
	if (hitbox_) {
		hitbox_->Apply();
	}
}


bool BossKnight::IsKnockbackImmune() const {
	// ボスはどの状態でものけぞらないが、これは「弾いた」演出を出すかどうかのフラグ
	// （ヘッダーのコメント参照）。踏み込みを止められない突進(Rush)中だけ true にする
	return currentState_ == states_.at(BossStateName::Rush).get();
}

const KnockbackResistance& BossKnight::GetKnockbackResistance() const {
	// 突進中は完全無効。踏み込みが鈍ると「見てから避ける」の読みが崩れる
	static const KnockbackResistance kImmune{ 1.0f, false, false, false };
	if (IsKnockbackImmune()) return kImmune;
	return Enemy::GetKnockbackResistance();
}

void BossKnight::UpdateBodyVisual(float deltaTime) {
	// アーマー中被弾フラッシュのタイマーを進める（アーマー解除後も残光が消えるまで減衰させる）
	if (armorHitFlashTimer_ > 0.0f) {
		armorHitFlashTimer_ -= deltaTime;
		if (armorHitFlashTimer_ < 0.0f) armorHitFlashTimer_ = 0.0f;
	}

	const bool armorActive = IsKnockbackImmune() && !IsAppearanceEffectPlaying();

	// 攻撃の溜め具合。振り抜いた瞬間に発光がパチッと消えないよう追従させる
	const float windupTarget = (boneAttack_->IsWindingUp() && !IsAppearanceEffectPlaying())
		? boneAttack_->GetWindupProgress() : 0.0f;
	float follow = kWindupGlowFollowRate * deltaTime;
	if (follow > 1.0f) follow = 1.0f;
	windupGlow_ += (windupTarget - windupGlow_) * follow;

	if (armorActive) {
		// ── 紫オーラ（体のメッシュ表面から発生）──
		auraEmitTimer_ += deltaTime;
		while (auraEmitTimer_ >= kAuraEmitInterval) {
			auraEmitter_->Emit();
			auraEmitTimer_ -= kAuraEmitInterval;
		}

		// ── 追従ライトを紫+増光にして周囲へ状態を知らせる ──
		if (characterLight_) {
			characterLight_->SetColorOverride({ 0.65f, 0.3f, 1.0f, 1.0f });
			characterLight_->SetIntensityScale(2.0f);
		}

		// ── 体と武器を紫に発光させる（ゆっくり脈動 + 被弾時に一瞬強く光る）──
		armorTintPhase_ += deltaTime;
		float pulse = 0.30f + 0.15f * std::sin(armorTintPhase_ * 10.0f);
		pulse += (armorHitFlashTimer_ / kArmorHitFlashDuration) * 0.6f; // 被弾フラッシュ
		const Vector4 tint = { 0.45f, 0.15f, 0.8f, pulse };

		if (auto* bodyRenderer = GetRenderer(name_)) {
			bodyRenderer->SetEmissiveTint(tint);
		}
	} else if (windupGlow_ > 0.01f) {
		// ── 攻撃の溜め: 体と追従ライトを橙に光らせ、振り抜く直前ほど強くする ──
		// 「今から攻撃が来る」を色と明るさで伝える。弾かれの紫とは別の色にして、
		// プレイヤーが「攻撃が通らない」ではなく「避けろ」と読めるようにする
		auraEmitTimer_ = 0.0f;
		armorTintPhase_ = 0.0f;

		// 二乗にして、溜めの終盤で一気に明るくなるカーブにする
		const float glow = windupGlow_ * windupGlow_;

		if (characterLight_) {
			characterLight_->SetColorOverride({ 1.0f, 0.45f, 0.12f, 1.0f });
			characterLight_->SetIntensityScale(1.0f + 2.5f * glow);
		}

		if (auto* bodyRenderer = GetRenderer(name_)) {
			bodyRenderer->SetEmissiveTint({ 1.0f, 0.3f, 0.05f, 0.8f * glow });
		}
	} else {
		// アーマーも溜めも無し: すべての表示を通常状態に戻す
		auraEmitTimer_ = 0.0f;
		armorTintPhase_ = 0.0f;

		if (characterLight_) {
			characterLight_->ClearColorOverride();
			characterLight_->SetIntensityScale(1.0f);
		}

		if (auto* bodyRenderer = GetRenderer(name_)) {
			bodyRenderer->SetEmissiveTint({ 0.0f, 0.0f, 0.0f, 0.0f });
		}
	}
}

void BossKnight::OnDeathEffectFinished() {
	// 攻撃判定を後始末する（本体は Enemy::Update の !isAlive_ 側で後始末される）
	if (hitbox_) {
		hitbox_->Deactivate();
		hitbox_->isAlive = false;
		hitbox_->ResetObject();
		hitbox_ = nullptr;
	}
}

void BossKnight::OnCollisionEnter(BaseCollider* other) {
	Enemy::OnCollisionEnter(other);

	if (other->category_ != CollisionCategory::PlayerWeapon) return;
	if (!player_ || !player_->IsAttack()) return;
	// 出現・死亡演出中は被弾処理をしない
	if (IsAppearanceEffectPlaying()) return;

	// ── 仕様書 §20 の実装フロー ──
	// ① 攻撃判定がヒット
	const AttackData atk = player_->GetAttackData(); // 値返しなのでローカルにコピー
	const float damage = atk.damage;
	const bool isArmorHit = IsKnockbackImmune();

	// 攻撃がヒットしたのでライトを強く光らせる
	FlashLight();
	// 弾いた場合は紫の発光（UpdateBodyVisual）で見せるので白フラッシュは出さない
	if (!isArmorHit) {
		PlayHitFlash();
	}

	// ② ダメージ計算（弾かれていてもダメージは通る）
	hp_ -= damage;
	RecordDamage(damage);

	// ③④ 方向と耐性。突進中は GetKnockbackResistance() が完全無効を返すので、
	//     解決後の power / verticalPower は 0 になる
	CombatHit::Attacker attacker;
	attacker.position = player_->GetWorldTransform()->GetTranslation();
	attacker.forward = player_->GetForward();
	const CombatHit::Result hit = CombatHit::Resolve(
		atk, attacker, GetWorldTransform()->GetTranslation(), GetKnockbackResistance());

	if (hp_ <= 0.0f) {
		if (CanDie()) {
			OnDeath();
			// 死亡演出中はステート更新が止まるため、吹き飛びの初速を直接与える。
			// 生きている間は耐性で動かせないので、とどめだけは **耐性を通さない** 攻撃の値で飛ばす
			const Vector3 awayFromPlayer =
				GetWorldTransform()->GetTranslation() - player_->GetWorldTransform()->GetTranslation();
			ApplyDeathLaunch(awayFromPlayer, CombatHit::ApplyTypeScale(atk.knockback));
			return;
		}
		// まだ死亡できない（トレーニングの敵無敵など）ので生存を維持する
		hp_ = 1.0f;
	}

	// ⑤⑥ ノックバック。
	// 耐性で canStagger が false なので **ステートは変わらず**、位置だけがわずかに押される。
	// ボスは長い予備動作を見て回避する相手なので、プレイヤーの攻撃で行動は中断されない
	ApplyKnockback(hit.info, hit.causesReaction);

	// ⑦⑧ ヒットストップと演出
	if (isArmorHit) {
		// 「弾かれた」感を出す: 通常より短いヒットストップ + 紫の硬い火花 + 体の紫フラッシュ
		// （通常のヒットエフェクトはあえて出さず、攻撃が通っていないことを伝える）
		hitStop_->Start(atk.hitStopTime * 0.35f, atk.hitStopIntensity, atk.hitStopStrength);
		armorHitEmitter_->Emit();
		armorHitFlashTimer_ = kArmorHitFlashDuration;
		return;
	}

	// 通常時のヒットストップ（手応えはここで返す）
	hitStop_->Start(atk.hitStopTime, atk.hitStopIntensity * 3.0f, atk.hitStopStrength);
}

void BossKnight::OnCollisionStay(BaseCollider* other) { Enemy::OnCollisionStay(other); }
void BossKnight::OnCollisionExit(BaseCollider* other) { Enemy::OnCollisionExit(other); }
