#include "Enemy.h"
#include <World3D/Object/Renderer/RendererManager.h>
#include <World3D/Object/Renderer/PrimitiveRenderer.h>
#include <World3D/Collider/CollisionManager.h>
#include <World3D/Object/Renderer/ModelRenderer.h>
#include "GameObject/Character/Player/Player.h"
#include <Scene/Transition/TransitionManager.h>


Enemy::Enemy(std::string objectName) : Object3d(objectName) {
	Object3d::Initialize();

}

Enemy::~Enemy() {
	lockOnTarget_.Finalize();
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
	}
}

void Enemy::Update(float deltaTime) {
	if (!player_) {
		player_ = static_cast<Player*>(Object3dManager::GetInstance().FindObject("Player"));
		GetCollider(name_)->category_ = CollisionCategory::Enemy;
	}

	// 起動する前だったら動かない（描画・影も止める）
	if (!isActive_) {
		SetIsDraw(false);
		if (characterLight_) characterLight_->SetEnabled(false);
		return;
	}
	SetIsDraw(true);

	if (!isAlive_) {
		isAlive = false;
		GetCollider(name_)->isAlive = false;
		GetRenderer(name_)->isAlive = false;
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
			Object3d::Update(deltaTime);
			onGround_ = false;
			return;
		}
		if (appearanceFx_->IsDying()) {
			// 死亡演出中: 意思決定を止め、ノックバックの慣性と重力だけを適用する
			velocity_.y += -9.8f * deltaTime;
			float damp = 1.0f - 2.0f * deltaTime;
			if (damp < 0.0f) damp = 0.0f;
			velocity_.x *= damp;
			velocity_.z *= damp;
			GetWorldTransform()->GetTranslation() += velocity_ * deltaTime;
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

	if (currentState_) {
		currentState_->Update(*this, dt);
	}

	GetWorldTransform()->GetTranslation() += velocity_ * dt;
	velocity_ += acceleration_ * dt;

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
	// 空中に配置されていても、真下の地面に接地した位置から出現させる
	SnapToGround();
	isActive_ = true;

	// 出現演出（黒い粒子が収束 + ディゾルブイン）を開始する
	if (appearanceFx_) {
		appearanceFx_->StartAppear();
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

#ifdef _DEBUG

void Enemy::DebugGui() {
	ImGui::Begin("Enemy");
	Object3d::DebugGui();
	ImGui::End();
}
#endif // _DEBUG


void Enemy::OnCollisionEnter(BaseCollider* other) {
	if (other->category_ != CollisionCategory::Ground) return;
	ResolveGroundCollision(other, /*resetVelocity=*/true);
}

void Enemy::OnCollisionStay(BaseCollider* other) {
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
		currentState_->Enter(*this);
	}
}

void Enemy::OnDeath() {
	// 死亡演出（黒い粒子を撒き散らし + ディゾルブアウト）を開始する。
	// 演出終了後に Enemy::Update 側で isAlive_ が false になる。
	if (appearanceFx_) {
		if (!appearanceFx_->IsDying() && !appearanceFx_->IsDeathFinished()) {
			appearanceFx_->StartDeath();
		}
		return;
	}

	isAlive_ = false;
}

void Enemy::SetupLockOn(LockOnSystem* lockOnSystem) {
	lockOnTarget_.Initialize(lockOnSystem, this);
}

