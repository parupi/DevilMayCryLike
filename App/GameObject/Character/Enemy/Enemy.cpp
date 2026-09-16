#include "Enemy.h"
#include <World3D/Object/Renderer/RendererManager.h>
#include <World3D/Object/Renderer/PrimitiveRenderer.h>
#include <World3D/Collider/CollisionManager.h>
#include <World3D/Collider/OBBCollider.h>
#include <World3D/Collider/AABBCollider.h>
#include <World3D/Collider/SphereCollider.h>
#include <World3D/Object/Renderer/ModelRenderer.h>
#include "GameObject/Character/Player/Player.h"
#include "GameObject/Character/Combat/CombatHitResolver.h"
#include "GameObject/Character/Enemy/EnemyStateNames.h"
#include <Scene/Transition/TransitionManager.h>
#include "Audio/SoundManager.h"
#include "Utility/TimeManager.h"
#include "World3D/Object/Model/Animation/AnimationPlayer.h"
#include "World3D/Object/Model/Animation/SkinnedInstance.h"
#include <algorithm>
#include <cmath>
#include <numbers>
#ifdef _DEBUG
#endif


Enemy::Enemy(std::string objectName) : Object3d(objectName) {
	Object3d::Initialize();

}

Enemy::~Enemy() {
	lockOnTarget_.Finalize();
}

void Enemy::RegisterStateClip(const std::string& stateName, const std::string& clipName,
	bool loop, float impactRatio) {
	stateClips_[stateName] = StateClip{ clipName, loop, impactRatio };
}

AnimationPlayer* Enemy::GetAnimationPlayer() {
	BaseRenderer* renderer = GetRenderer(name_);
	if (!renderer) return nullptr;
	// 静的モデルのままの敵はスキンインスタンスを持たないので、その場合は素通りさせる
	SkinnedInstance* instance = renderer->GetSkinnedInstance();
	return instance ? instance->GetPlayer() : nullptr;
}

void Enemy::BeginAttackAnimation(float weaponImpactSeconds) {
	attackFitSeconds_ = weaponImpactSeconds;
	attackAnimRestart_ = true;
}

// 出現・死亡演出とステートから再生クリップを決める。
// AnimationPlayer::Play は同じクリップなら何もしないので毎フレーム呼んでよい。
// 攻撃は「武器の振りの長さ」にクリップを詰めて、体と武器がずれないようにしている。
void Enemy::UpdateAnimation() {
	AnimationPlayer* anim = GetAnimationPlayer();
	if (!anim) return;

	// ── 出現・死亡は専用クリップを最優先。1回流すものは演出の長さに合わせる ──
	// ループ指定（出現専用モーションが無くて待機で代用する場合）は等速のまま。
	// minSpeed を渡すと「遅くする側」だけ止める（尺より早く終わったクリップは最後のポーズで止まる）
	auto playEffectClip = [anim](const StateClip& entry, float effectSeconds, float blendTime, float minSpeed) {
		anim->Play(entry.clip, entry.loop, blendTime);
		const float duration = anim->GetDuration();
		const bool fit = !entry.loop && duration > 0.01f && effectSeconds > 0.01f;
		anim->SetSpeed(fit ? (std::max)(duration / effectSeconds, minSpeed) : 1.0f);
	};

	if (appearanceFx_) {
		if (appearanceFx_->IsAppearing() && !spawnClip_.clip.empty()) {
			playEffectClip(spawnClip_, appearanceFx_->GetAppearDuration(), 0.0f, 0.0f);
			return;
		}
		if ((appearanceFx_->IsDying() || appearanceFx_->IsDeathFinished()) && !deathClip_.clip.empty()) {
			// 死亡クリップは「吹き飛び + 死亡モーション」の尺で流し切る。
			// 残りの段階（黒いもや・ディゾルブ）は倒れたポーズのまま見せる
			playEffectClip(deathClip_, appearanceFx_->GetDeathClipDuration(), 0.1f, kDeathClipSpeedMin);
			return;
		}
	}

	// ── 通常時はステート名で決める。未登録なら今のクリップを続ける ──
	auto it = stateClips_.find(currentStateName_);
	if (it == stateClips_.end()) {
		return;
	}

	anim->Play(it->second.clip, it->second.loop, 0.15f, attackAnimRestart_);
	attackAnimRestart_ = false;

	// 攻撃中だけ、体の「振り切る瞬間」が武器の振り抜きと重なるように再生速度を決める。
	// GetDuration() は再生中クリップの長さなので必ず Play の後に取ること
	if (attackSpeedOverride_ > 0.0f) {
		// ステート側が速度を明示している間はそちらに従う（予備動作の引き伸ばしなど）
		anim->SetSpeed(attackSpeedOverride_);
	} else if (attackFitSeconds_ > 0.01f) {
		const float clipImpact = anim->GetDuration() * it->second.impactRatio;
		const float speed = (clipImpact > 0.01f) ? (clipImpact / attackFitSeconds_) : 1.0f;
		anim->SetSpeed(std::clamp(speed, kAttackSpeedMin, kAttackSpeedMax));
	} else {
		anim->SetSpeed(1.0f);
	}
}

void Enemy::ApplyModelRotation() {
	BaseRenderer* renderer = GetRenderer(name_);
	if (!renderer) {
		return;
	}
	// 行ベクトル規約なので v * M(補正) * M(リアクション) = v * M(リアクション * 補正)。
	// 先にモデルを正面へ向けてから、被弾でよろけさせる。
	renderer->GetWorldTransform()->GetRotation() = modelReactionRotation_ * modelRotationOffset_;
}

void Enemy::ApplyModelGroundOffset() {
	BaseRenderer* renderer = GetRenderer(name_);
	if (!renderer) return;
	BaseCollider* collider = GetCollider(name_);
	if (!collider) return;

	// オブジェクトの原点はコライダーの中心なので、モデルをそのまま置くと
	// 縦半分ぶん宙に浮いてしまう（影だけが地面に落ちて見える）。
	// Player は kModelOffsetY で同じことを手で書いているが、敵はコライダーの大きさが
	// ステージ側のデータで決まるため、ここで引き直して合わせる。
	// 値はすべてオブジェクトのローカル単位（配置スケールは親の行列が掛ける）
	float bottomLocal = 0.0f;
	switch (collider->GetShapeType()) {
	case CollisionShapeType::OBB: {
		const OBBData& data = static_cast<OBBCollider*>(collider)->GetColliderData();
		// OBBCollider::Update は offset を「回転はするが拡大しない」＝ワールド単位で足し、
		// halfExtents だけを配置スケールで拡大する。ここはローカル単位で揃えるので offset だけスケールで割る。
		// 割らないと、コライダーを上へずらして2倍に置いたボス（本編の BossDragon）で
		// モデルが offset ぶん（1.92m）宙に浮いていた
		const float objScaleY = GetWorldTransform()->GetWorldScale().y;
		const float offsetLocalY = (objScaleY > 1.0e-4f) ? (data.offset.y / objScaleY) : data.offset.y;
		bottomLocal = offsetLocalY - data.halfExtents.y;
		break;
	}
	case CollisionShapeType::AABB:
		bottomLocal = static_cast<AABBCollider*>(collider)->GetColliderData().offsetMin.y;
		break;
	case CollisionShapeType::Sphere: {
		const SphereData& data = static_cast<SphereCollider*>(collider)->GetColliderData();
		bottomLocal = data.offset.y - data.radius;
		break;
	}
	}

	// 接地中はコライダーの底が地面より kGroundSink だけ下にあるので、そのぶん持ち上げて
	// 足の裏を地面の高さに合わせる。沈み量はワールド単位なので配置スケールで割る
	const float scaleY = GetWorldTransform()->GetWorldScale().y;
	const float sinkLocal = (scaleY > 1.0e-4f) ? (kGroundSink / scaleY) : 0.0f;

	// 死亡モーションで倒れた体が地面まで届かないモデルは、倒れるのに合わせて沈める。
	// 死亡クリップ（吹き飛び + 死亡モーション）の長さをかけて滑らかに下げ、以降はそのまま保つ
	float deathSink = 0.0f;
	if (deathModelSink_ > 0.0f && appearanceFx_ &&
		(appearanceFx_->IsDying() || appearanceFx_->IsDeathFinished())) {
		const float clipSeconds = appearanceFx_->GetDeathClipDuration();
		const float t = (clipSeconds > 0.01f) ? std::clamp(deathSinkTimer_ / clipSeconds, 0.0f, 1.0f) : 1.0f;
		deathSink = deathModelSink_ * (t * t * (3.0f - 2.0f * t));
	}

	renderer->GetWorldTransform()->GetTranslation().y = bottomLocal + sinkLocal + modelGroundOffset_ - deathSink;
}

void Enemy::Initialize() {
	if (!hitStop_) {
		hitStop_ = std::make_unique<HitStop>();
	}

	// 出現・死亡演出（本体のレンダラーをディゾルブ対象に登録する）
	if (!appearanceFx_) {
		appearanceFx_ = std::make_unique<EnemyAppearanceEffect>();
		appearanceFx_->Initialize(this);
		if (auto* renderer = GetRenderer(name_)) {
			appearanceFx_->AddRenderer(renderer);
		}
	}

	// 被弾時に体を白く光らせるコンポーネント（本体のレンダラーを対象に登録する）
	if (!hitFlash_) {
		hitFlash_ = std::make_unique<HitFlashComponent>();
		if (auto* renderer = GetRenderer(name_)) {
			hitFlash_->AddRenderer(renderer);
		}
	}

	// 敵に追従するポイントライト（被弾時にフラッシュする）
	if (!characterLight_) {
		characterLight_ = std::make_unique<CharacterLight>();
		characterLight_->Initialize(name_ + "Light", Vector4{ 1.0f, 0.3f, 0.15f, 1.0f });
		// 出現前に光らないよう消灯しておく（Update内で状態に応じて点灯する）
		characterLight_->SetEnabled(false);
	}

	auto it = states_.find("Air");
	if (it != states_.end()) {
		currentState_ = it->second.get();
		// アニメーション選択にも使うので名前を合わせておく。
		// 派生クラスは最後に ChangeState() で本来の初期ステートへ移るが、
		// 忘れてもバインドポーズのまま固まらないようにここで入れておく
		currentStateName_ = "Air";
	}
}

void Enemy::Update(float deltaTime) {
	if (!player_) {
		player_ = static_cast<Player*>(Object3dManager::GetInstance().FindObject("Player"));
		// コライダーはステージデータ側で付ける。エディタで付け忘れても落ちないようにする
		for (BaseCollider* collider : GetColliders()) {
			collider->category_ = CollisionCategory::Enemy;
		}
	}

	// 起動する前だったら動かない（描画・影も止める）
	if (!isActive_) {
		SetIsDraw(false);
		if (characterLight_) characterLight_->SetEnabled(false);
		// 出現前でも「配置された位置」にワールド行列を作っておく。
		// ここを飛ばすと行列が単位行列のままになり、子であるコライダーが
		// ワールド原点に取り残される。原点に大きな当たり判定ができて
		// プレイヤーの行動を邪魔するうえ、その押し出しで未出現の敵の座標が
		// 毎フレーム流され、ステージを保存すると壊れた位置が焼き付く。
		// 描画はしないのでレンダラー（スキニング）は回さず、行列だけ作る
		UpdateTransformOnly();
		// 出現前は判定も切る（見えない敵に攻撃が当たる・押し返されるのを防ぐ）
		SetCollidersActive(false);
		return;
	}
	SetIsDraw(true);

	if (!isAlive_) {
		isAlive = false;
		for (BaseCollider* collider : GetColliders()) {
			collider->isAlive = false;
		}
		for (BaseRenderer* renderer : GetRenderers()) {
			renderer->isAlive = false;
		}
		if (characterLight_) characterLight_->SetEnabled(false);
		return;
	}

	// モデルの足元を地面に合わせる。出現・死亡演出中も含めて毎フレーム掛け直す
	// （コライダーの大きさはステージデータ側なので、エディタで変えても追従させたい）
	ApplyModelGroundOffset();

	// キャラクター追従ライトの更新（出現・死亡演出中も含めて追従させる）
	if (characterLight_) {
		characterLight_->SetEnabled(true);
		characterLight_->Update(GetWorldTransform()->GetTranslation(), deltaTime);
	}

	// ── 出現・死亡演出 ──
	if (appearanceFx_) {
		appearanceFx_->Update(deltaTime);

		if (appearanceFx_->IsAppearing()) {
			// 出現演出中: ディゾルブで実体化し終わるまで行動しない
			UpdateAnimation();
			Object3d::Update(deltaTime);
			onGround_ = false;
			return;
		}
		if (appearanceFx_->IsDying()) {
			// 死亡演出中: 意思決定を止め、とどめの吹き飛びの慣性と重力だけを適用する。
			// 演出はここから「吹き飛び → 死亡モーション → 黒いもや → ディゾルブ」と進むので、
			// 倒れた体が地面で震えないよう接地したら落下速度を殺しておく
			// （経過時間は ApplyModelGroundOffset の死体の沈み込みに使う）
			deathSinkTimer_ += deltaTime;
			if (onGround_) {
				if (velocity_.y < 0.0f) {
					velocity_.y = 0.0f;
				}
			} else {
				velocity_.y += -9.8f * deltaTime;
			}
			float damp = 1.0f - 2.0f * deltaTime;
			if (damp < 0.0f) damp = 0.0f;
			velocity_.x *= damp;
			velocity_.z *= damp;
			GetWorldTransform()->GetTranslation() += velocity_ * deltaTime;
			ClampToMovementBounds();
			// とどめの一撃の白フラッシュを最後まで再生させる（止めると白いまま固まる）
			if (hitFlash_) {
				hitFlash_->Update(deltaTime);
			}
			UpdateAnimation();
			Object3d::Update(deltaTime);
			onGround_ = false;
			return;
		}
		if (appearanceFx_->IsDeathFinished()) {
			// 演出が終わったので本当に死亡させる（次のUpdateでコライダー・レンダラーが無効化される）
			OnDeathEffectFinished();
			isAlive_ = false;
			return;
		}
	}

	hitStop_->Update(deltaTime);
	float dt = deltaTime * hitStop_->GetTimeScale();
	// パーティクルなど自分でヒットストップを持たない系統にも時間停止を伝える
	TimeManager::RequestGameTimeScale(hitStop_->GetTimeScale());

	// 被弾フラッシュ。派生クラスの見た目更新（ボスのアーマー発光など）より後に走らせる必要があるため、
	// 派生の Update から Enemy::Update が呼ばれるこの位置で更新する。
	// dt（ヒットストップ適用後）で進めるので、時間が止まっている間は白いまま保持される。
	if (hitFlash_) {
		hitFlash_->Update(dt);
	}

	// ノックバックの速度を先に進める（仕様書 §6・§7）。
	// ステートより先に更新することで、被弾リアクションのステートは今フレームの値を読める。
	// 接地は前フレームの押し出し結果（Update の最後で落とし、衝突コールバックが立て直す）
	knockback_.SetGrounded(onGround_);
	knockback_.Update(dt, kGravity);

	// 行動停止中は意思決定のステートを回さず、水平方向の自走も止める。
	// 被弾リアクションと落下（IsReaction）は最後まで再生させないと、
	// のけぞりの傾きが戻らない・空中で固まる、といった見た目の破綻になる
	const bool holdStill = actionSuppressed_ && currentState_ && !currentState_->IsReaction();
	if (holdStill) {
		velocity_.x = 0.0f;
		velocity_.z = 0.0f;
	} else if (currentState_) {
		currentState_->Update(*this, dt);
	}

	// ステートが確定してからクリップを決める（この下に early return があるのでここで呼ぶ）
	UpdateAnimation();

	// 仕様書 §6: velocity = moveVelocity + knockbackVelocity。
	// ノックバックを別の速度として足すので、のけぞらない敵（ボス）でも位置だけは押される
	GetWorldTransform()->GetTranslation() += (velocity_ + knockback_.GetVelocity()) * dt;
	velocity_ += acceleration_ * dt;

	// 追跡・突進・ノックバックのどれで動いた場合もここを通るので、まとめて範囲内に収める
	ClampToMovementBounds();

	if (!player_) {
		return;
	}

	// 攻撃の振り始めから終わりまでは向きを固定する（EnemyAttackAim が LockFacing で止める）。
	// ここで向き直ると、予兆が消えた後に横へ動いたプレイヤーの方へ攻撃が曲がってしまう
	if (!facingLocked_) {
		// ローカル +Z をプレイヤーと反対側へ向ける（＝前方向のローカル -Z がプレイヤーを向く）
		TurnToward(GetWorldTransform()->GetTranslation() - player_->GetWorldTransform()->GetTranslation(), dt);
	}

	Object3d::Update(dt);

	onGround_ = false;
}

void Enemy::Draw() {
	if (isActive_ && isAlive_) {
		Object3d::Draw();
	}
}

void Enemy::DrawEffect() {
}

Vector3 Enemy::GetForward() {
	// 敵はローカル -Z がプレイヤー側を向く（Enemy::Update の回転）
	Vector3 forward = TransformNormal({ 0.0f, 0.0f, -1.0f }, GetWorldTransform()->GetMatWorld());
	forward.y = 0.0f;
	if (Length(forward) < 0.001f) return { 0.0f, 0.0f, 1.0f };
	return Normalize(forward);
}

void Enemy::LockFacing(const Vector3& forward) {
	facingLocked_ = true;

	Vector3 flat{ forward.x, 0.0f, forward.z };
	// 向きが取れないとき（真上を向いている等）は今の向きのまま止める
	if (Length(flat) < 0.001f) return;

	// Update の向き直りと同じ式。ローカル +Z を forward の反対へ向ける
	// ＝ 前方向（ローカル -Z）が forward を向く。
	// 固定は向き直りの速さに関係なく一瞬で揃える（予兆がその向きで出ているため）
	faceDir_ = Normalize(-flat);
	GetWorldTransform()->GetRotation() = FromToRotation({ 0.0f, 0.0f, 1.0f }, faceDir_);
}

void Enemy::TurnToward(const Vector3& away, float deltaTime) {
	Vector3 target{ away.x, 0.0f, away.z };
	// 真上・真下に居るなど向きが決まらないときは、今の向きのまま
	if (Length(target) < 0.001f) return;
	target = Normalize(target);

	const float turnSpeed = (faceTurnSpeedOverride_ > 0.0f) ? faceTurnSpeedOverride_ : faceTurnSpeed_;
	if (turnSpeed > 0.0f && Length(faceDir_) > 0.5f) {
		// 今の向きから目標までの角度を、このフレームに回ってよい角度で切る
		constexpr float kPi = std::numbers::pi_v<float>;
		const float currentYaw = std::atan2(faceDir_.x, faceDir_.z);
		float delta = std::atan2(target.x, target.z) - currentYaw;
		// -π〜π に畳んで、近い方へ回る
		if (delta > kPi) delta -= 2.0f * kPi;
		if (delta < -kPi) delta += 2.0f * kPi;
		const float maxStep = turnSpeed * (kPi / 180.0f) * deltaTime;
		const float yaw = currentYaw + std::clamp(delta, -maxStep, maxStep);
		target = { std::sin(yaw), 0.0f, std::cos(yaw) };
	}

	faceDir_ = target;
	GetWorldTransform()->GetRotation() = FromToRotation({ 0.0f, 0.0f, 1.0f }, target);
}

Vector3 Enemy::GetFootPosition() {
	Vector3 pos = GetWorldTransform()->GetWorldPos();

	// 敵はY軸まわりにしか回らないので、コライダーの底は「中心 - 縦の半分」でよい
	BaseCollider* collider = GetCollider(name_);
	if (!collider) return pos;

	switch (collider->GetShapeType()) {
	case CollisionShapeType::OBB: {
		auto* obb = static_cast<OBBCollider*>(collider);
		pos.y = obb->GetCenter().y - obb->GetWorldHalfExtents().y;
		break;
	}
	case CollisionShapeType::AABB: {
		auto* aabb = static_cast<AABBCollider*>(collider);
		pos.y = aabb->GetMin().y;
		break;
	}
	case CollisionShapeType::Sphere: {
		auto* sphere = static_cast<SphereCollider*>(collider);
		pos.y = sphere->GetCenter().y - sphere->GetRadius();
		break;
	}
	}
	return pos;
}

Vector3 Enemy::GetBodyCenter() {
	BaseCollider* collider = GetCollider(name_);
	if (!collider) return GetWorldTransform()->GetWorldPos();

	switch (collider->GetShapeType()) {
	case CollisionShapeType::OBB:
		return static_cast<OBBCollider*>(collider)->GetCenter();
	case CollisionShapeType::AABB: {
		auto* aabb = static_cast<AABBCollider*>(collider);
		return (aabb->GetMin() + aabb->GetMax()) * 0.5f;
	}
	case CollisionShapeType::Sphere:
		return static_cast<SphereCollider*>(collider)->GetCenter();
	}
	return GetWorldTransform()->GetWorldPos();
}

void Enemy::Spawn() {
	// 出現前に切ってあった判定を戻す。SnapToGround が自分のコライダーを使うので先に済ませる
	SetCollidersActive(true);
	// 空中に配置されていても、真下の地面に接地した位置から出現させる
	SnapToGround();
	isActive_ = true;

	// 出現演出（黒い粒子が収束 + ディゾルブイン）を開始する
	if (appearanceFx_) {
		appearanceFx_->StartAppear();
	}

	// 出現音。敵の位置で鳴らすので、背後に湧いたことが音でも分かる
	if (const char* se = GetSpawnSound(); se && *se) {
		SoundManager::GetInstance().PlaySE3D(se, GetWorldTransform()->GetTranslation(), 0.9f);
	}
}

void Enemy::ClampToMovementBounds() {
	if (!hasMovementBounds_) return;

	Vector3& pos = GetWorldTransform()->GetTranslation();
	Vector3 inwardNormal{};
	if (!movementBounds_.ClampPosition(pos, inwardNormal)) return;

	// 壁の外を向いている速度成分を取り除く。
	// （残すとノックバックの慣性で壁に押し付けられ続けてしまう）
	const float outwardSpeed = Dot(velocity_, inwardNormal);
	if (outwardSpeed < 0.0f) {
		velocity_ -= inwardNormal * outwardSpeed;
	}

	// ノックバックの速度も同じように削る（仕様書 §10）。
	// 初速のほうも削らないと、時間ベースの減衰カーブから次のフレームに同じ速度が復活する
	if (knockback_.CancelOutward(inwardNormal)) {
		// 壁ヒット演出は1回のノックバックにつき1度だけ（押し付けられている間ずっと鳴らさない）
		if (!knockbackWallHit_) {
			knockbackWallHit_ = true;
			OnKnockbackWallHit();
		}
	}
}

void Enemy::SnapToGround() {
	BaseCollider* myCol = GetCollider(name_);
	if (!myCol) {
		Object3d::Update(0.0f);
		return;
	}

	const Vector3 originalPos = GetWorldTransform()->GetTranslation();
	const auto& colliders = CollisionManager::GetInstance().GetColliders();

	// 少しずつ下へ移動しながら、Groundコライダーへのめり込みを検出する
	const float step = 0.3f;
	const int maxSteps = 500; // 最大150m下まで探索

	for (int i = 0; i < maxSteps; ++i) {
		// 本体→コライダーの順にワールド行列を更新してから判定する
		Object3d::Update(0.0f);
		myCol->Update();

		for (const auto& other : colliders) {
			if (!other || other.get() == myCol) continue;
			if (!other->isAlive) continue;
			if (other->category_ != CollisionCategory::Ground) continue;

			PenetrationResult result = CollisionManager::GetInstance().CalculatePenetration(myCol, other.get());
			if (result.hit && result.normal.y > 0.5f) {
				// ResolveGroundCollision と同じ押し出し + わずかな沈み込み
				GetWorldTransform()->GetTranslation() += result.normal * result.depth;
				GetWorldTransform()->GetTranslation().y -= kGroundSink;
				onGround_ = true;
				Object3d::Update(0.0f);
				return;
			}
		}

		GetWorldTransform()->GetTranslation().y -= step;
	}

	// 地面が見つからなかったら元の位置に戻す
	GetWorldTransform()->GetTranslation() = originalPos;
	Object3d::Update(0.0f);
}



void Enemy::SetCollidersActive(bool active) {
	for (BaseCollider* collider : GetColliders()) {
		if (collider) collider->SetColliderActive(active);
	}
}

void Enemy::OnCollisionEnter(BaseCollider* other) {
	// 出現前は押し出されない（判定を切ってあるので届かないはずだが、配置位置を守るための保険）
	if (!isActive_) return;
	if (other->category_ != CollisionCategory::Ground) return;
	ResolveGroundCollision(other);
}

void Enemy::OnCollisionStay(BaseCollider* other) {
	if (!isActive_) return;
	if (other->category_ != CollisionCategory::Ground) return;
	ResolveGroundCollision(other);
}

void Enemy::ResolveGroundCollision(BaseCollider* other) {
	BaseCollider* enemyCollider = GetCollider(name_);
	if (!enemyCollider) return;

	PenetrationResult result = CollisionManager::GetInstance().CalculatePenetration(enemyCollider, other);
	if (!result.hit) return;

	GetWorldTransform()->GetTranslation() += result.normal * result.depth;

	if (result.normal.y > 0.5f) {
		GetWorldTransform()->GetTranslation().y -= kGroundSink;

		// 地面に載っている間は下向きの速度を残さない。
		// 残すと「毎フレーム沈み込む → 押し出される」を繰り返して上下にがくつく。
		// 特に吹き飛ばしの着地後が目立つ（KnockBack が落下速度を書き込んだまま
		// 意思決定ステートへ戻り、そちらは velocity_.y を触らないため残り続ける）。
		// 上向きの速度は消さない（打ち上げた瞬間はまだ地面と重なっているので、
		// ここで消すと飛び上がれなくなる）
		if (velocity_.y < 0.0f) {
			velocity_.y = 0.0f;
		}

		// ノックバックで浮いていたなら、ここが着地。
		// 落下速度を消して水平を弱め、滑って止まる形にする（仕様書 §11 の「復帰」）
		if (knockback_.IsAirborne() && knockback_.GetVelocity().y <= 0.0f) {
			knockback_.OnLand();
		}
		knockback_.SetGrounded(true);

		onGround_ = true;
	}

	// 押し出した結果を自分のコライダーへ反映する。
	// CollisionManager は判定を回す前に一度だけコライダーを更新するので、これが無いと
	// 同じフレームの2枚目以降の床が「まだ深くめり込んでいる」古い位置で計算され、
	// 押し出しが二重に効いて地面から浮く → 落ちる → また浮く、を繰り返す。
	// （床が複数のコライダーに分かれている場所へ吹き飛ばすとこれが起きる）
	UpdateTransformOnly();
	enemyCollider->Update();
}

void Enemy::OnCollisionExit(BaseCollider* other) {
	other;
}

void Enemy::ChangeState(const std::string& stateName) {
	if (currentState_) currentState_->Exit(*this);

	auto it = states_.find(stateName);
	if (it != states_.end()) {
		currentState_ = it->second.get();
		// アニメーションの選択に使う。currentState_ はポインタなので名前は別に控えておく
		currentStateName_ = stateName;
		currentState_->Enter(*this);
	}
}

void Enemy::OnDeath() {
	// 撃破スコアは一度だけ加算する（OnDeathは死亡演出中に複数回呼ばれ得る）
	if (!killScored_) {
		killScored_ = true;
		if (player_) {
			if (auto* score = player_->GetScoreManager()) {
				score->OnEnemyKilled(GetStyleMultiplier());
			}
		}
	}

	// 死亡音。killScored_ と同じくここを複数回通るので、1回目だけ鳴らす
	// （このブロックの直前で killScored_ が立っているので、それを目印にはできない）
	if (!deathSoundPlayed_) {
		deathSoundPlayed_ = true;
		if (const char* se = GetDeathSound(); se && *se) {
			SoundManager::GetInstance().PlaySE3D(se, GetWorldTransform()->GetTranslation(), 1.0f);
		}
	}

	// 死亡演出中の動きは velocity_ 側で受け持つので、ノックバックの速度は捨てる
	knockback_.Stop();

	// 倒れた体はもう戦闘の相手ではないので、当たり判定のカテゴリを外す。
	// （判定自体は残す＝地面との接地判定に要る。Enemy 側は相手のカテゴリしか見ないので支障はない）
	// これを外さないと、演出中の死体にプレイヤーが押される・死体を斬ってスコアが入る、が起きる
	for (BaseCollider* collider : GetColliders()) {
		if (collider) collider->category_ = CollisionCategory::None;
	}

	// 死亡演出（吹き飛び → 死亡モーション → 黒いもや → ディゾルブ）を開始する。
	// 演出終了後に Enemy::Update 側で isAlive_ が false になる。
	if (appearanceFx_) {
		if (!appearanceFx_->IsDying() && !appearanceFx_->IsDeathFinished()) {
			appearanceFx_->StartDeath();
		}
		return;
	}

	isAlive_ = false;
}

void Enemy::OnKnockbackWallHit() {
	// 壁にぶつかったことを最小限伝える（仕様書 §10 の「壁ヒット演出」）。
	// 壁バウンド・壁張り付きは Wall カテゴリのコライダーが要るのでまだ入れていない
	FlashLight();
	PlayHitFlash();
}

void Enemy::ApplyKnockback(const DamageInfo& info, bool causesReaction) {
	pendingDamageInfo_ = info;
	knockbackWallHit_ = false;

	// 仕様書 §8: すでにノックバック中なら、攻撃ごとの合成方法（上書き / 加算+上限）に従う。
	// 止まっているところへの1発目は、今の移動速度を引き継ぐかどうかを overrideVelocity で決める
	if (knockback_.IsActive()) {
		knockback_.AddHit(info.knockback, info.direction);
	} else {
		knockback_.Begin(info.knockback, info.direction, velocity_);
	}

	// 浮かせる攻撃を受けたら、その場で接地を解除して本体も少し持ち上げる。
	// ここで浮かせておかないと、押し出し（ResolveGroundCollision）が同じフレームに
	// 接地を立て直して「もう着地した」と判断され、初速が消える。
	// のけぞりは地上では浮かないので対象外（持ち上げると当たるたびに小さく跳ねる）
	if (info.knockback.type != ReactionType::HitStun && info.knockback.verticalPower > 0.0f) {
		GetWorldTransform()->GetTranslation().y += kGroundSink;
		onGround_ = false;
	}

	if (!causesReaction) {
		// のけぞらない相手（ボスなど）。行動は中断せず、速度だけを受けて押される
		return;
	}

	// 意思決定のステートを止めて被弾リアクションへ。
	// すでにリアクション中なら入れ直さない（入れ直すと傾き・回転が毎ヒットで巻き戻る）
	if (currentStateName_ != EnemyStateName::KnockBack && HasState(EnemyStateName::KnockBack)) {
		ChangeState(EnemyStateName::KnockBack);
	}
}

void Enemy::ApplyDeathLaunch(const Vector3& direction, const KnockbackData& knockback) {
	// とどめの吹き飛びは死亡演出（ステート更新が止まる）の中で動かすので、
	// ノックバックの部品ではなく velocity_ へ直接書く
	knockback_.Stop();

	// 水平方向は攻撃の向きに従う。真上・真下成分は落として斜めに飛びすぎないようにする
	Vector3 horizontalDir{ direction.x, 0.0f, direction.z };
	if (Length(horizontalDir) > 0.001f) {
		horizontalDir = Normalize(horizontalDir);
	} else {
		horizontalDir = {};
	}

	// 空中コンボのように吹き飛ばしが 0 の攻撃でとどめを刺しても、
	// 「倒した」ことが分かるように最低限の初速は出す
	const float horizontalSpeed = (std::max)(knockback.power, kDeathLaunchMinSpeed);
	const float verticalSpeed = (std::max)(knockback.verticalPower, kDeathLaunchMinUpSpeed);

	velocity_ = horizontalDir * horizontalSpeed;
	velocity_.y = verticalSpeed;
	onGround_ = false;
}

void Enemy::SetupLockOn(LockOnSystem* lockOnSystem) {
	lockOnTarget_.Initialize(lockOnSystem, this);
}

