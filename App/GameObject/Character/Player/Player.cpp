#include "Player.h"
#include "State/PlayerStateIdle.h"
#include "State/PlayerStateMove.h"
#include "3d/Object/Renderer/RendererManager.h"
#include "3d/Object/Renderer/PrimitiveRenderer.h"
#include "3d/Collider/AABBCollider.h"
#include "3d/Collider/CollisionManager.h"
#include "base/utility/DeltaTime.h"
#include "State/PlayerStateJump.h"
#include "State/PlayerStateAir.h"
#include "3d/Primitive/PrimitiveLineDrawer.h"
#include "State/Attack/PlayerStateAttack.h"
#include "scene/Transition/TransitionManager.h"
#include "State/PlayerStateDeath.h"
#include "State/PlayerStateClear.h"
#include "Controller/PlayerInput.h"
#include "2d/SpriteManager.h"

#include <numbers>

#include "input/Input.h"

Player::Player(std::string objectNama) : Object3d(objectNama) {
	Object3d::Initialize();

	// レンダラーの生成
	RendererManager::GetInstance()->AddRenderer(std::make_unique<ModelRenderer>("PlayerHead", "PlayerHead"));

	AddRenderer(RendererManager::GetInstance()->FindRender("PlayerHead"));

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
}

void Player::Initialize() {
	combat_ = std::make_unique<PlayerCombat>();
	combat_->Initialize(this);

	GetCollider(name_)->category_ = CollisionCategory::Player;
	static_cast<AABBCollider*>(GetCollider(name_))->GetColliderData().offsetMax *= 0.5f;
	static_cast<AABBCollider*>(GetCollider(name_))->GetColliderData().offsetMin *= 0.5f;
	// 武器のレンダラー生成
	RendererManager::GetInstance()->AddRenderer(std::make_unique<ModelRenderer>("PlayerWeapon", "Sword"));
	// 武器用のコライダー生成
	CollisionManager::GetInstance()->AddCollider(std::make_unique<AABBCollider>("WeaponCollider"));
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

	hitStop_ = std::make_unique<HitStop>();
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

	weapon_->Update(dt);
	// 攻撃中でないならステートを更新する
	if (!combat_->IsAttacking()) {
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
}

void Player::Draw() {
	weapon_->Draw();
	Object3d::Draw();

}

void Player::DrawEffect() {

	combat_->Draw();
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
	// ステートの更新
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

void Player::OnCollisionEnter(BaseCollider* other) {
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
