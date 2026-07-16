#include "Player.h"
#include "State/PlayerStateIdle.h"
#include "State/PlayerStateMove.h"
#include "World3D/Object/Renderer/RendererManager.h"
#include "World3D/Object/Renderer/PrimitiveRenderer.h"
#include "World3D/Collider/AABBCollider.h"
#include "World3D/Collider/OBBCollider.h"
#include "World3D/Collider/CollisionManager.h"
#include "Utility/DeltaTime.h"
#include "State/PlayerStateJump.h"
#include "State/PlayerStateAir.h"
#include "World3D/Primitive/PrimitiveLineDrawer.h"
#include "State/Attack/PlayerStateAttack.h"
#include "Scene/Transition/TransitionManager.h"
#include "State/PlayerStateDeath.h"
#include "State/PlayerStateClear.h"
#include "State/PlayerStateKnockBack.h"
#include "Controller/PlayerInput.h"
#include "Graphics/Rendering/Sprite/SpriteManager.h"

#include <numbers>

#include "Input/Input.h"

Player::Player(std::string objectName) : Object3d(objectName) {
	Object3d::Initialize();

	// レンダラーの生成
	RendererManager::GetInstance().AddRenderer(std::make_unique<ModelRenderer>("PlayerHead", "PlayerHead"));

	AddRenderer(RendererManager::GetInstance().FindRender("PlayerHead"));
	// 地面にしっかりつくようにする
	GetRenderer("PlayerHead")->GetWorldTransform()->GetTranslation().y -= 0.5f;

	// StateMachine生成
	stateMachine_ = std::make_unique<PlayerStateMachine>();
	// ステートをセットしてcurrentに登録
	stateMachine_->SetFirstState("Idle", std::make_unique<PlayerStateIdle>());
	// 各ステートを登録
	stateMachine_->AddState("Move", std::make_unique<PlayerStateMove>());
	stateMachine_->AddState("Jump", std::make_unique<PlayerStateJump>());
	stateMachine_->AddState("Air", std::make_unique<PlayerStateAir>());
	stateMachine_->AddState("Death", std::make_unique<PlayerStateDeath>());
	stateMachine_->AddState("Clear", std::make_unique<PlayerStateClear>());
	stateMachine_->AddState("Knockback", std::make_unique<PlayerStateKnockBack>());
}

void Player::Initialize() {
	combat_ = std::make_unique<PlayerCombat>();
	combat_->Initialize(this);

	GetCollider(name_)->category_ = CollisionCategory::Player;
	static_cast<OBBCollider*>(GetCollider(name_))->GetColliderData().halfExtents *= 0.5f;
	// 武器のレンダラー生成
	RendererManager::GetInstance().AddRenderer(std::make_unique<ModelRenderer>("PlayerWeapon", "Sword"));
	// 武器用のコライダー生成
	CollisionManager::GetInstance().AddCollider(std::make_unique<OBBCollider>("WeaponCollider"));
	// 武器を生成
	weapon_ = std::make_unique<PlayerWeapon>("PlayerWeapon");
	// 武器にレンダラーを追加
	weapon_->AddRenderer(RendererManager::GetInstance().FindRender("PlayerWeapon"));
	// 武器にコライダーを追加
	weapon_->AddCollider(CollisionManager::GetInstance().FindCollider("WeaponCollider"));
	// 武器の初期化
	weapon_->Initialize();
	// 武器のワールドトランスフォームをプレイヤーの子にする
	weapon_->GetWorldTransform()->SetParent(GetWorldTransform());
	// プレイヤークラスのポインタを武器クラスに渡す
	weapon_->SetPlayer(this);

	scoreManager = std::make_unique<StylishScoreManager>();

	weapon_->SetScoreManager(scoreManager.get());

	// HP表示用のハートスプライトを生成
	hearts_.resize(maxHp_);
	for (int32_t i = 0; i < maxHp_; ++i) {
		// UI要素の初期化
		hearts_[i] = SpriteManager::GetInstance().CreateSprite(SpriteLayer::UI, "heart" + std::to_string(i), "Heart.png");
		hearts_[i]->SetSize({64.0f, 64.0f});
		hearts_[i]->SetAnchorPoint({0.5f, 0.5f});
		hearts_[i]->SetPosition({50.0f + i * 50.0f, 50.0f}); // 左上に横並びで配置
	}

	hitStop_ = std::make_unique<HitStop>();

	hitVignette_ = std::make_unique<HitVignetteEffect>();
	hitVignette_->Initialize();
}

void Player::Update(float deltaTime) {
	hitStop_->Update(deltaTime);
	float dt = deltaTime * hitStop_->GetTimeScale();

	// Rキーを押したら死亡演出が流れる ← デバッグ用
	if (Input::GetInstance().TriggerKey(DIK_R)) {
		ChangeState("Death");
	}

	scoreManager->Update();

	LockOn();

	// 無敵時間のカウントダウン
	if (invincibleTimer_ > 0.0f) {
		invincibleTimer_ -= dt;
	}

	// 被弾ビネットの更新
	hitVignette_->Update(dt);

	weapon_->Update(dt);

	// ノックバック中は戦闘状態に関わらず常にステートを更新する
	bool isKnockbackState = (stateMachine_->GetCurrentState() && std::string(stateMachine_->GetCurrentState()->GetDebugName()) == "Knockback");
	if (isKnockbackState) {
		stateMachine_->UpdateCurrentState(*this, dt);
	} else if (!combat_->IsAttacking()) {
		stateMachine_->UpdateCurrentState(*this, dt);
	}
	combat_->Update(dt);
	// 入力に応じたコマンドを実行
	for (auto& cmd : input_->GetCommands()) {
		ExecuteCommand(cmd);
	}

	// 移動処理
	GetWorldTransform()->GetTranslation() += velocity_ * dt;
	velocity_ += acceleration_ * dt;

	Object3d::Update(dt);

	// 強制戦闘エリアなどで移動範囲が設定されている場合はXZを範囲内にクランプする
	if (hasMovementBounds_) {
		Vector3& pos = GetWorldTransform()->GetTranslation();
		if (pos.x < movementBoundsMin_.x) pos.x = movementBoundsMin_.x;
		else if (pos.x > movementBoundsMax_.x) pos.x = movementBoundsMax_.x;
		if (pos.z < movementBoundsMin_.z) pos.z = movementBoundsMin_.z;
		else if (pos.z > movementBoundsMax_.z) pos.z = movementBoundsMax_.z;
	}

	// 接地フラグを毎フレーム切っておく
	onGround_ = false;

	for (size_t i = 0; i < hearts_.size(); ++i) {
		if (i < hp_) {
			hearts_[i]->SetColor({1.0f, 1.0f, 1.0f, 1.0f}); // HP分は表示
		} else {
			hearts_[i]->SetColor({0.0f, 0.0f, 0.0f, 0.3f}); // 残りは半透明で表示
		}
		hearts_[i]->Update();
	}
}

void Player::Draw() {
	weapon_->Draw();
	Object3d::Draw();

}

void Player::DrawEffect() {
	combat_->Draw();
	weapon_->DrawEffect();
}

void Player::DrawUI() {

}

#ifdef _DEBUG
void Player::DebugGui() {
	stateMachine_->DebugGui();
	ImGui::Begin("Player");
	Object3d::DebugGui();
	ImGui::End();
}

#endif // _DEBUG

void Player::ChangeState(const std::string& stateName) {
	stateMachine_->ChangeState(*this, stateName);
}

void Player::ExecuteCommand(const PlayerCommand& command) {
	// デス状態では全コマンドを無視する
	auto* cur = stateMachine_->GetCurrentState();
	if (cur && std::string(cur->GetDebugName()) == "Death") return;

	if (!combat_->IsAttacking()) {
		stateMachine_->ExecuteCommand(*this, command);
	}
	combat_->ExecuteCommand(command);
}

Vector3 Player::GetMoveDirection() const {
	BaseCamera* camera = CameraManager::GetInstance().GetActiveCamera();
	if (!camera) return {};
	// 入力のcontext
	const PlayerInputContext& context = input_->GetContext();
	// 移動中じゃなければ処理しない
	if (!context.isMove) return {};

	// 入力方向を取得
	Vector3 inputDir = {context.move.x, 0.0f, context.move.y};
	// 大きさが小さければ処理しない
	if (Length(inputDir) < 0.01f) return {};
	// 入力方向を正規化
	inputDir = Normalize(inputDir);
	// カメラ基準で移動させる
	// カメラの前方向を取得
	Vector3 camForward = camera->GetForward();
	camForward.y = 0.0f;
	camForward = Normalize(camForward);
	// カメラの右方向を取得
	Vector3 camRight = camera->GetRight();
	camRight.y = 0.0f;
	camRight = Normalize(camRight);
	// カメラの向きと入力方向から移動方向を作成
	Vector3 moveDir = camRight * inputDir.x + camForward * inputDir.z;
	// 正規化して返す
	return Normalize(moveDir);
}

void Player::Move(Vector3 moveDir, float) {
	// y軸の速度はそのままにしておく
	float velocityY = velocity_.y;

	velocity_ = moveDir * moveSpeed_;
	velocity_.y = velocityY;
}

void Player::Rotate(Vector3 moveDir, float deltaTime) {
	// ロックオンしているなら回転させない
	if (lockOn_->IsLockOn()) return;
	// 移動方向が無ければ処理しない
	if (Length(moveDir) < 0.001f) return;

	moveDir.x *= -1.0f;
	// 移動方向から目標の回転を作成
	Quaternion targetRot = LookRotation(moveDir);
	// 現在の回転を取得
	Quaternion& currentRot = GetWorldTransform()->GetRotation();
	// 補完して回転を更新
	currentRot = Slerp(currentRot, targetRot, rotateSpeed_ * deltaTime);
}

void Player::LockOn() {
	if (lockOn_->IsLockOn()) {
		// 現在のターゲットを取得
		auto* target = lockOn_->GetCurrentTarget();
		// ターゲットへのベクトルを計算
		Vector3 toTarget = target->GetWorldPosition() - GetWorldTransform()->GetTranslation();

		// ターゲット方向に向く
		Vector3 direction = Normalize(toTarget);
		direction.x *= -1.0f;
		Quaternion lookRot = LookRotation(direction);
		// 回転を適用
		GetWorldTransform()->GetRotation() = lookRot;
	}
}

void Player::TakeDamage(const DamageInfo& info) {
	// デス状態・無敵時間中は被ダメージなし
	auto* cur = stateMachine_->GetCurrentState();
	if (cur && std::string(cur->GetDebugName()) == "Death") return;
	if (invincibleTimer_ > 0.0f) return;

	hp_ -= static_cast<int32_t>(info.damage);
	invincibleTimer_ = 1.2f;

	pendingDamageInfo_ = info;

	// 被弾ビネットフラッシュ
	hitVignette_->Play();

	// 攻撃を強制中断
	combat_->InterruptCombat();

	if (hp_ <= 0.0f) {
		ChangeState("Death");
		hitVignette_->Stop();
	} else {
		stateMachine_->ChangeState(*this, "Knockback");
	}
}

void Player::OnCollisionEnter(BaseCollider* other) {
	// 敵の武器に当たったら被ダメージ
	if (other->category_ == CollisionCategory::EnemyWeapon) {
		if (!other->owner_) return;

		Vector3 weaponPos = other->owner_->GetWorldTransform()->GetTranslation();
		Vector3 playerPos = GetWorldTransform()->GetTranslation();
		Vector3 dir = playerPos - weaponPos;
		dir.y = 0.0f;
		dir = (Length(dir) > 0.001f) ? Normalize(dir) : Vector3{0.0f, 0.0f, -1.0f};

		DamageInfo info;
		info.damage = 1.0f;
		info.direction = dir;
		info.type = ReactionType::Knockback;
		info.impulseForce = 15.0f;
		info.upwardRatio = 0.4f;
		info.stunTime = 0.7f;
		info.hitPosition = playerPos;
		info.attackerPosition = weaponPos;

		TakeDamage(info);
		return;
	}

	if (other->category_ == CollisionCategory::Ground || other->category_ == CollisionCategory::Enemy) {
		ResolveGroundCollision(other);
	}
}

void Player::OnCollisionStay(BaseCollider* other) {
	if (other->category_ == CollisionCategory::Ground || other->category_ == CollisionCategory::Enemy) {
		ResolveGroundCollision(other);
	}
}

void Player::ResolveGroundCollision(BaseCollider* other) {
	BaseCollider* playerCollider = GetCollider("Player");

	PenetrationResult result = CollisionManager::GetInstance().CalculatePenetration(playerCollider, other);
	if (!result.hit) return;

	GetWorldTransform()->GetTranslation() += result.normal * result.depth;

	if (result.normal.y > 0.5f) {
		// 上に当たってる（接地）
		GetWorldTransform()->GetTranslation().y -= 0.1f;
		//velocity_.y = 0.0f;
		onGround_ = true;
	}
}

void Player::OnCollisionExit(BaseCollider* other) {
	other;
}
