#include "GruntMelee.h"
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
#include "GameObject/Character/Enemy/State/EnemyStateKnockBack.h"
#include "Graphics/Rendering/Particle/ParticleManager.h"
#ifdef _DEBUG
#endif

#include "State/GruntStatePatrol.h"
#include "State/GruntStateCombatIdle.h"
#include "State/GruntStateApproach.h"
#include "State/GruntStateSideMove.h"
#include "State/GruntStateRetreat.h"
#include "State/GruntStateAttackNormal.h"
#include "State/GruntStateRushAttack.h"

GruntMelee::GruntMelee(std::string objectName) : Enemy(objectName) {
	// ModelRenderer は FindModel するだけで読み込みはしないので、ここで読んでおく。
	// 以前は TitleScene が先読みしていたが、他シーンの読み込みに依存すると
	// そちらを整理したときに静かに壊れるので、Ground / Prop と同じく自分で読む
	// Skeleton.gltf はリグ付き（12ジョイント・5クリップ）なのでスキンモデルとして読む。
	// ここで LoadModel してしまうと ModelManager::FindModel が静的モデルを先に返して
	// 黙ってアニメーションしなくなるので注意
	ModelManager::GetInstance().LoadSkinnedModel(kModelName);
	ModelManager::GetInstance().LoadModel("Sword");
	RendererManager::GetInstance().AddRenderer(std::make_unique<ModelRenderer>(name_, kModelName));
	AddRenderer(RendererManager::GetInstance().FindRender(name_));
	// Skeleton.obj は素の高さが約5m。プレイヤー（約0.9m）より少し背が高い程度に縮める
	GetRenderer(name_)->GetWorldTransform()->GetScale() = { kModelScale, kModelScale, kModelScale };
	// このモデルは正面が +Z。敵の前方向はローカル -Z なので180度回す（詳細は Enemy::SetModelRotationOffset）
	SetModelRotationOffset(EulerDegree({ 0.0f, 180.0f, 0.0f }));

	hp_ = 8.0f;
	maxHp_ = hp_;

	// ノックバック耐性（仕様書 §9 の「小型」）。
	// 軽攻撃も強攻撃も打ち上げも素通しで、コンボの練習台になる敵
	SetKnockbackResistance(KnockbackResistance{
		/*resistance=*/ 0.0f,
		/*canStagger=*/ true,
		/*canBlowAway=*/ true,
		/*canLaunch=*/ true,
	});
}

void GruntMelee::Initialize() {
	// レベルデータのコライダーサイズをそのまま使う
	// （旧: レベル側の×2補正を打ち消すための *0.5f があったが、×2補正の撤廃に伴い削除。実効サイズは変わらない）

	// 武器の生成
	RendererManager::GetInstance().AddRenderer(std::make_unique<ModelRenderer>(name_ + "Weapon", "Sword"));
	CollisionManager::GetInstance().AddCollider(std::make_unique<AABBCollider>(name_ + "Weapon"));
	// 武器コライダーカテゴリを設定（プレイヤーへのダメージ判定に使用）
	CollisionManager::GetInstance().FindCollider(name_ + "Weapon")->category_ = CollisionCategory::EnemyWeapon;

	auto weapon = std::make_unique<GruntMeleeWeapon>(name_ + "Weapon");
	weapon->AddRenderer(RendererManager::GetInstance().FindRender(name_ + "Weapon"));
	weapon->AddCollider(CollisionManager::GetInstance().FindCollider(name_ + "Weapon"));
	weapon->Initialize();
	weapon->GetWorldTransform()->SetParent(GetWorldTransform());

	weapon_ = weapon.get();
	Object3dManager::GetInstance().AddObject(std::move(weapon));

	// コンポーネント生成
	sensor_ = std::make_unique<EnemySensorComponent>();
	movement_ = std::make_unique<EnemyMovementComponent>();
	meleeAttack_ = std::make_unique<EnemyMeleeAttackComponent>(weapon_);

	// ステート登録
	states_[EnemyStateName::Air] = std::make_unique<EnemyStateAir>();
	states_[EnemyStateName::KnockBack] = std::make_unique<EnemyStateKnockBack>();

	states_[GruntMeleeStateName::Patrol] = std::make_unique<GruntStatePatrol>(sensor_.get());
	// KnockBack/Air 終了後に EnemyStateName::Idle/Move へ戻るため CombatIdle を全キーで登録する
	states_[EnemyStateName::Idle] = std::make_unique<GruntStateCombatIdle>(sensor_.get());
	states_[EnemyStateName::Move] = std::make_unique<GruntStateCombatIdle>(sensor_.get());
	states_[GruntMeleeStateName::CombatIdle] = std::make_unique<GruntStateCombatIdle>(sensor_.get());
	states_[GruntMeleeStateName::Approach] = std::make_unique<GruntStateApproach>(movement_.get());
	states_[GruntMeleeStateName::SideMove] = std::make_unique<GruntStateSideMove>(movement_.get());
	states_[GruntMeleeStateName::Retreat] = std::make_unique<GruntStateRetreat>(movement_.get());
	states_[GruntMeleeStateName::AttackNormal] = std::make_unique<GruntStateAttackNormal>(meleeAttack_.get());
	states_[GruntMeleeStateName::RushAttack] = std::make_unique<GruntStateRushAttack>(meleeAttack_.get());

	currentState_ = states_[GruntMeleeStateName::Patrol].get();

	// ── アニメーションの割り当て（Skeleton.gltf の5クリップ）──
	// 攻撃クリップは EnemyMeleeAttackComponent が武器の振りの長さに合わせて伸縮させる
	RegisterStateClip(GruntMeleeStateName::Patrol,       kClipIdle);
	RegisterStateClip(EnemyStateName::Idle,              kClipIdle);
	RegisterStateClip(EnemyStateName::Move,              kClipIdle);
	RegisterStateClip(GruntMeleeStateName::CombatIdle,   kClipIdle);
	RegisterStateClip(GruntMeleeStateName::Approach,     kClipRun);
	RegisterStateClip(GruntMeleeStateName::SideMove,     kClipRun);
	RegisterStateClip(GruntMeleeStateName::Retreat,      kClipRun);
	RegisterStateClip(GruntMeleeStateName::AttackNormal, kClipAttack, false, kAttackImpactRatio);
	RegisterStateClip(GruntMeleeStateName::RushAttack,   kClipAttack, false, kAttackImpactRatio);
	RegisterStateClip(EnemyStateName::Air,               kClipIdle);
	// のけぞり・吹き飛びは専用クリップが無いので待機のまま。
	// 傾き/回転（SetModelReactionRotation）の方でリアクションを見せている
	RegisterStateClip(EnemyStateName::KnockBack,         kClipIdle);
	SetSpawnClip(kClipSpawn);
	SetDeathClip(kClipDeath);

	// 被弾時のヒットエフェクトは HitEffectSystem の "HitImpact" に一本化したのでここでは持たない。
	// （足元から出る旧エフェクトと違い、武器が実際に当たった位置へ火花とリングが出る）

	// パーティクル: チャージエフェクト（予備動作中に収束するリング）
	ParticleManager::GetInstance().CreateEmitter(name_ + "ChargeEffect");
	chargeEmitter_ = ParticleManager::GetInstance().GetEmitters().at(name_ + "ChargeEffect").get();
	chargeEmitter_->SetParent(GetWorldTransform());
	chargeEmitter_->AddParticle("EnemyChargeRing");

	Enemy::Initialize();

	// 出現・死亡演出のディゾルブ対象に武器も含める
	if (appearanceFx_) {
		appearanceFx_->AddRenderer(RendererManager::GetInstance().FindRender(name_ + "Weapon"));
	}

	// 初期ステートを Patrol へ上書き（Enemy::Initialize() は "Air" にセットする）
	ChangeState(GruntMeleeStateName::Patrol);
}

void GruntMelee::Update(float deltaTime) {
	// 死亡演出終了後は武器が後始末済みのため、本体の後始末だけ行う
	if (!IsAlive()) {
		Enemy::Update(deltaTime);
		return;
	}

	if (isActive_) {
		// 武器は常に表示し、攻撃中以外はデフォルトポーズに戻す
		weapon_->SetIsDraw(true);
	} else {
		// 未出現時は武器を隠し、判定も切る（本体と同じく、出現前に殴られないようにする）
		weapon_->SetIsDraw(false);
		if (auto* weaponCol = weapon_->GetCollider(name_ + "Weapon")) {
			weaponCol->SetColliderActive(false);
		}
		// 非アクティブ時の共通処理（消灯・配置位置へのワールド行列更新）
		Enemy::Update(deltaTime);
		return;
	}


	if (meleeAttack_->IsFinished()) {
		// -Z が敵の前方向。Y は Skeleton モデルの手の高さに合わせている
		// （立方体だった頃は 0.1f で、そのままだと足元に剣が浮く）
		weapon_->GetWorldTransform()->GetTranslation() = { 0.0f, 0.45f, -0.5f };
		weapon_->GetWorldTransform()->GetRotation()    = EulerDegree({ 0.0f, 90.0f, 150.0f });
	}

	// 攻撃フェーズのみ武器コライダーを有効化（予備動作・非攻撃時・出現/死亡演出中は無効）
	bool isAttackPhase = !meleeAttack_->IsFinished() && !meleeAttack_->IsWindingUp()
		&& !IsAppearanceEffectPlaying();
	auto* weaponCol = static_cast<AABBCollider*>(weapon_->GetCollider(name_ + "Weapon"));
	if (weaponCol) {
		weaponCol->GetColliderData().isActive = isAttackPhase;
	}

	// 予備動作中にチャージリングを一定間隔で発射。
	// 死亡演出中はステートが回らず攻撃の時間も止まるので、構えのまま死んでも出さない
	if (meleeAttack_->IsWindingUp() && !IsAppearanceEffectPlaying()) {
		chargeEmitTimer_ += deltaTime;
		if (chargeEmitTimer_ >= kChargeEmitInterval) {
			chargeEmitter_->Emit();
			chargeEmitTimer_ = 0.0f;
		}
	} else {
		chargeEmitTimer_ = 0.0f;
	}

	Enemy::Update(deltaTime);
}


void GruntMelee::OnCollisionEnter(BaseCollider* other) {
	Enemy::OnCollisionEnter(other);

	if (other->category_ != CollisionCategory::PlayerWeapon) return;
	if (!player_ || !player_->IsAttack()) return;
	// 出現・死亡演出中は被弾処理をしない
	if (IsAppearanceEffectPlaying()) return;

	// ── 仕様書 §20 の実装フロー ──
	// ① 攻撃判定がヒット
	const AttackData atk = player_->GetAttackData(); // 値返しなのでローカルにコピー

	// ② ダメージ計算
	hp_ -= atk.damage;
	RecordDamage(atk.damage);

	// ③④ ノックバックの方向と耐性。CombatHit が水平化・種類補正・耐性をまとめて解決する
	CombatHit::Attacker attacker;
	attacker.position = player_->GetWorldTransform()->GetTranslation();
	attacker.forward = player_->GetForward();
	const CombatHit::Result hit = CombatHit::Resolve(
		atk, attacker, GetWorldTransform()->GetTranslation(), GetKnockbackResistance());

	if (hp_ <= 0.0f) {
		if (CanDie()) {
			// 死亡演出中はステートが回らないので、構えの途中で倒されると攻撃が残る。
			// ボスと同じく攻撃ごと中断する（予兆は閃光なしで消え、向きの固定も解ける）
			meleeAttack_->Cancel(*this);
			OnDeath();
			// 死亡演出中はステート更新が止まるため、吹き飛びの初速を直接与える
			ApplyDeathLaunch(hit.info.direction, hit.info.knockback);
			return;
		} else {
			// まだ死亡できない（チュートリアル中など）ので生存を維持する
			hp_ = 1.0f;
		}
	}

	// ⑤⑥ ノックバック速度と被弾リアクション。
	// 空中で追撃を受けたときの合成（落下を止める・打ち上げ直す）は KnockbackComponent の担当
	ApplyKnockback(hit.info, hit.causesReaction);

	// ⑦ ヒットストップ。**ノックバックより後に開始する**のが仕様書 §13 の順番だが、
	// ヒットストップ中は Enemy::Update の dt が縮むので、実際に敵が動き出すのは停止が明けてから
	hitStop_->Start(atk.hitStopTime, atk.hitStopIntensity * 3.0f, atk.hitStopStrength);

	// ⑧ 演出（ライトと白フラッシュ。VFX・SE・カメラは PlayerWeapon 側の HitEffectSystem）
	FlashLight();
	PlayHitFlash();
}

void GruntMelee::OnDeathEffectFinished() {
	// 武器を後始末する（本体は Enemy::Update の !isAlive_ 側で後始末される）
	if (weapon_) {
		weapon_->isAlive = false;
		weapon_->ResetObject();
		weapon_ = nullptr;
	}
}

void GruntMelee::OnCollisionStay(BaseCollider* other) {
	Enemy::OnCollisionStay(other);
}

void GruntMelee::OnCollisionExit(BaseCollider* other) {
	Enemy::OnCollisionExit(other);
}
