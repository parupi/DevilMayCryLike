#include "Player.h"
#include "State/PlayerStateIdle.h"
#include "State/PlayerStateMove.h"
#include "3d/Object/Renderer/RendererManager.h"
#include "3d/Object/Renderer/PrimitiveRenderer.h"
#include "3d/Collider/AABBCollider.h"
#include "3d/Collider/OBBCollider.h"
#include "3d/Collider/CollisionManager.h"
#include "base/utility/DeltaTime.h"
#include "State/PlayerStateJump.h"
#include "State/PlayerStateAir.h"
#include "3d/Primitive/PrimitiveLineDrawer.h"
#include "State/Attack/PlayerStateAttack.h"
#include "scene/Transition/TransitionManager.h"
#include "State/PlayerStateDeath.h"
#include "State/PlayerStateClear.h"
#include "State/PlayerStateKnockBack.h"
#include "Controller/PlayerInput.h"
#include "2d/SpriteManager.h"

#include <numbers>

#include "input/Input.h"

Player::Player(std::string objectNama) : Object3d(objectNama) {
	Object3d::Initialize();

	// レンダラーの生成
	RendererManager::GetInstance()->AddRenderer(std::make_unique<ModelRenderer>("PlayerHead", "PlayerHead"));

	AddRenderer(RendererManager::GetInstance()->FindRender("PlayerHead"));
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
	static_cast<AABBCollider*>(GetCollider(name_))->GetColliderData().offsetMax *= 0.5f;
	static_cast<AABBCollider*>(GetCollider(name_))->GetColliderData().offsetMin *= 0.5f;
	//static_cast<AABBCollider*>(GetCollider(name_))->transform_->GetTranslation().y += 0.1f;
	// 武器のレンダラー生成
	RendererManager::GetInstance()->AddRenderer(std::make_unique<ModelRenderer>("PlayerWeapon", "Sword"));
	// 武器用のコライダー生成
	CollisionManager::GetInstance()->AddCollider(std::make_unique<OBBCollider>("WeaponCollider"));
	// 武器を生成
	weapon_ = std::make_unique<PlayerWeapon>("PlayerWeapon");
	// 武器にレンダラーを追加
	weapon_->AddRenderer(RendererManager::GetInstance()->FindRender("PlayerWeapon"));
	// 武器にコライダーを追加
	weapon_->AddCollider(CollisionManager::GetInstance()->FindCollider("WeaponCollider"));
	// 武器の初期化
	weapon_->Initialize();
	// 武器のワールドトランスフォームをプレイヤーの子にする
	weapon_->GetWorldTransform()->SetParent(GetWorldTransform());
	// プレイヤークラスのポインタを武器クラスに渡す
	weapon_->SetPlayer(this);

	scoreManager = std::make_unique<StylishScoreManager>();

	weapon_->SetScoreManager(scoreManager.get());

	reticle_ = SpriteManager::GetInstance()->CreateSprite(SpriteLayer::Game, "reticle", "reticle.png");
	reticle_->SetSize({ 32.0f, 32.0f });
	reticle_->SetAnchorPoint({ 0.5f, 0.5f });
	// HP表示用のハートスプライトを生成
	hearts_.resize(maxHp_);
	for (int32_t i = 0; i < maxHp_; ++i) {
		// UI要素の初期化
		hearts_[i] = SpriteManager::GetInstance()->CreateSprite(SpriteLayer::UI, "heart" + std::to_string(i), "Heart.png");
		hearts_[i]->SetSize({ 64.0f, 64.0f });
		hearts_[i]->SetAnchorPoint({ 0.5f, 0.5f });
		hearts_[i]->SetPosition({ 50.0f + i * 50.0f, 50.0f }); // 左上に横並びで配置
	}

	hitStop_ = std::make_unique<HitStop>();

	hitVignette_ = std::make_unique<HitVignetteEffect>();
	hitVignette_->Initialize();
}

void Player::Update(float deltaTime) {
	hitStop_->Update(deltaTime);
	float dt = deltaTime * hitStop_->GetTimeScale();

	// Rキーを押したら死亡演出が流れる ← デバッグ用
	if (Input::GetInstance()->TriggerKey(DIK_R)) {
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
	bool isKnockbackState = (stateMachine_->GetCurrentState() &&
		std::string(stateMachine_->GetCurrentState()->GetDebugName()) == "Knockback");
	if (isKnockbackState) {
		stateMachine_->UpdateCurrentState(*this, dt);
	}
	else if (!combat_->IsAttacking()) {
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

	// 接地フラグを毎フレーム切っておく
	onGround_ = false;

	if (lockOn_->IsLockOn()) {
		reticle_->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
	}
	else {
		reticle_->SetColor({ 1.0f, 1.0f, 1.0f, 0.0f });
	}

	if (lockOn_->IsLockOn()) {
		reticle_->SetPosition(CameraManager::GetInstance()->GetCurrentCamera()->WorldToScreen(lockOn_->GetCurrentTarget()->GetWorldPosition(), 1280, 720));
		reticle_->Update();
	}

	for (size_t i = 0; i < hearts_.size(); ++i) {
		if (i < hp_) {
			hearts_[i]->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f }); // HP分は表示
		} else {
			hearts_[i]->SetColor({ 0.0f, 0.0f, 0.0f, 0.3f }); // 残りは半透明で表示
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

	//ImGui::Begin("Weapon");
	//weapon_->DebugGui();
	//ImGui::End();
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
	BaseCamera* camera = CameraManager::GetInstance()->GetActiveCamera();
	if (!camera) return {};
	// 入力のcontext
	const PlayerInputContext& context = input_->GetContext();
	// 移動中じゃなければ処理しない
	if (!context.isMove) return {};

	// 入力方向を取得
	Vector3 inputDir = { context.move.x, 0.0f, context.move.y };
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

void Player::Move(Vector3 moveDir, float deltaTime) {
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
		auto* target = lockOn_->GetCurrentTarget();

		Vector3 toTarget = target->GetWorldPosition() - GetWorldTransform()->GetTranslation();

		// ターゲット方向に向く
		Vector3 direction = Normalize(toTarget);
		direction.x *= -1.0f;
		Quaternion lookRot = LookRotation(direction);

		GetWorldTransform()->GetRotation() = lookRot;
	}
}

void Player::TakeDamage(const DamageInfo& info) {
	// デス状態・無敵時間中は被ダメージなし
	auto* cur = stateMachine_->GetCurrentState();
	if (cur && std::string(cur->GetDebugName()) == "Death") return;
	if (invincibleTimer_ > 0.0f) return;

	hp_ -= info.damage;
	invincibleTimer_ = 1.2f;

	pendingDamageInfo_ = info;

	// 被弾ビネットフラッシュ
	hitVignette_->Play();

	// 攻撃を強制中断
	combat_->InterruptCombat();

	if (hp_ <= 0.0f) {
		ChangeState("Death");
		hitVignette_->Stop();
	}
	else {
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
		dir = (Length(dir) > 0.001f) ? Normalize(dir) : Vector3{ 0.0f, 0.0f, -1.0f };

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

		AABBCollider* playerCollider = static_cast<AABBCollider*>(GetCollider("Player"));

		AABBCollider* blockCollider = static_cast<AABBCollider*>(other);

		Vector3 outNormal = AABBCollider::CalculateCollisionNormal(playerCollider, blockCollider);

		Vector3 playerMin = playerCollider->GetMin();
		Vector3 playerMax = playerCollider->GetMax();

		float playerOffset = (playerMax.x - playerMin.x) * 0.5f;

		if (outNormal.x == 1.0f) {
			// 右に当たってる
			GetWorldTransform()->GetTranslation().x = blockCollider->GetMax().x + playerOffset;
		}
		else if (outNormal.x == -1.0f) {
			// 左に当たってる
			GetWorldTransform()->GetTranslation().x = blockCollider->GetMin().x - playerOffset;
		}
		else if (outNormal.y == 1.0f) {
			// 上に当たってる
			GetWorldTransform()->GetTranslation().y = blockCollider->GetMax().y + playerOffset;
			GetWorldTransform()->GetTranslation().y -= 0.1f;
			//velocity_.y = 0.0f;
			onGround_ = true;
		}
		else if (outNormal.y == -1.0f) {
			// 下に当たってる
			GetWorldTransform()->GetTranslation().y = blockCollider->GetMin().y - playerOffset;
			//velocity_ *= -1.0f;
		}
		else if (outNormal.z == 1.0f) {
			// 奥に当たってる
			GetWorldTransform()->GetTranslation().z = blockCollider->GetMax().z + playerOffset;
		}
		else if (outNormal.z == -1.0f) {
			// 手前に当たってる
			GetWorldTransform()->GetTranslation().z = blockCollider->GetMin().z - playerOffset;
		}
	}
}

void Player::OnCollisionStay(BaseCollider* other) {
	if (other->category_ == CollisionCategory::Ground || other->category_ == CollisionCategory::Enemy) {

		AABBCollider* playerCollider = static_cast<AABBCollider*>(GetCollider("Player"));

		AABBCollider* blockCollider = static_cast<AABBCollider*>(other);

		Vector3 outNormal = AABBCollider::CalculateCollisionNormal(playerCollider, blockCollider);

		Vector3 playerMin = playerCollider->GetMin();
		Vector3 playerMax = playerCollider->GetMax();

		float playerOffset = (playerMax.x - playerMin.x) * 0.5f;

		if (outNormal.x == 1.0f) {
			// 右に当たってる
			GetWorldTransform()->GetTranslation().x = blockCollider->GetMax().x + playerOffset;
		}
		else if (outNormal.x == -1.0f) {
			// 左に当たってる
			GetWorldTransform()->GetTranslation().x = blockCollider->GetMin().x - playerOffset;
		}
		else if (outNormal.y == 1.0f) {
			// 上に当たってる
			GetWorldTransform()->GetTranslation().y = blockCollider->GetMax().y + playerOffset;
			GetWorldTransform()->GetTranslation().y -= 0.1f;
			//velocity_.y = 0.0f;
			onGround_ = true;
		}
		else if (outNormal.y == -1.0f) {
			// 下に当たってる
			GetWorldTransform()->GetTranslation().y = blockCollider->GetMin().y - playerOffset;
			//velocity_ *= -1.0f;
		}
		else if (outNormal.z == 1.0f) {
			// 奥に当たってる
			GetWorldTransform()->GetTranslation().z = blockCollider->GetMax().z + playerOffset;
		}
		else if (outNormal.z == -1.0f) {
			// 手前に当たってる
			GetWorldTransform()->GetTranslation().z = blockCollider->GetMin().z - playerOffset;
		}
	}
}

void Player::OnCollisionExit(BaseCollider* other) {
	other;
}
