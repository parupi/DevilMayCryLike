#include "Enemy.h"
#include <World3D/Object/Renderer/RendererManager.h>
#include <World3D/Object/Renderer/PrimitiveRenderer.h>
#include <World3D/Collider/CollisionManager.h>
#include <World3D/Object/Renderer/ModelRenderer.h>
#include "GameObject/Character/Player/Player.h"
#include <Scene/Transition/TransitionManager.h>
#include "Utility/TimeManager.h"
#include "World3D/Object/Model/Animation/AnimationPlayer.h"
#include "World3D/Object/Model/Animation/SkinnedInstance.h"
#include <algorithm>
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
	if (attackFitSeconds_ > 0.01f) {
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

	GetWorldTransform()->GetTranslation() += velocity_ * dt;
	velocity_ += acceleration_ * dt;

	// 追跡・突進・ノックバックのどれで動いた場合もここを通るので、まとめて範囲内に収める
	ClampToMovementBounds();

	if (!player_) {
		return;
	}

	// 座標取得
	Vector3 enemyPos = GetWorldTransform()->GetTranslation();
	Vector3 playerPos = player_->GetWorldTransform()->GetTranslation();

	// 敵 → プレイヤー方向
	Vector3 dir = enemyPos - playerPos;
	dir.y = 0.0f; // 上下は無視
	Normalize(dir);

	// 前方向（モデルの前が +Z 前提）
	Vector3 forward(0.0f, 0.0f, 1.0f);

	// 回転Quaternionを計算
	Quaternion rot = FromToRotation(forward, dir);

	// 回転をセット
	GetWorldTransform()->GetRotation() = rot;

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
				GetWorldTransform()->GetTranslation().y -= 0.1f;
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
	ResolveGroundCollision(other, /*resetVelocity=*/true);
}

void Enemy::OnCollisionStay(BaseCollider* other) {
	if (!isActive_) return;
	if (other->category_ != CollisionCategory::Ground) return;
	ResolveGroundCollision(other, /*resetVelocity=*/false);
}

void Enemy::ResolveGroundCollision(BaseCollider* other, bool resetVelocity) {
	BaseCollider* enemyCollider = GetCollider(name_);

	PenetrationResult result = CollisionManager::GetInstance().CalculatePenetration(enemyCollider, other);
	if (!result.hit) return;

	GetWorldTransform()->GetTranslation() += result.normal * result.depth;

	if (result.normal.y > 0.5f) {
		GetWorldTransform()->GetTranslation().y -= 0.1f;
		if (resetVelocity) velocity_.y = 0.0f;
		onGround_ = true;
	}
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

void Enemy::ApplyDeathLaunch(const Vector3& direction, float impulseForce, float upwardRatio) {
	// 水平方向は攻撃の向きに従う。真上・真下成分は落として斜めに飛びすぎないようにする
	Vector3 horizontalDir{ direction.x, 0.0f, direction.z };
	if (Length(horizontalDir) > 0.001f) {
		horizontalDir = Normalize(horizontalDir);
	} else {
		horizontalDir = {};
	}

	// 空中コンボのように吹き飛ばしが 0 の攻撃でとどめを刺しても、
	// 「倒した」ことが分かるように最低限の初速は出す
	const float horizontalSpeed = (std::max)(impulseForce, kDeathLaunchMinSpeed);
	const float verticalSpeed = (std::max)(impulseForce * upwardRatio, kDeathLaunchMinUpSpeed);

	velocity_ = horizontalDir * horizontalSpeed;
	velocity_.y = verticalSpeed;
	onGround_ = false;
}

void Enemy::SetupLockOn(LockOnSystem* lockOnSystem) {
	lockOnTarget_.Initialize(lockOnSystem, this);
}

