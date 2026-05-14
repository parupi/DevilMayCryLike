#include "Player.h"
#include "State/PlayerStateIdle.h"
#include "State/PlayerStateMove.h"
#include <3d/Object/Renderer/RendererManager.h>
#include <3d/Object/Renderer/PrimitiveRenderer.h>
#include <3d/Collider/AABBCollider.h>
#include <3d/Collider/CollisionManager.h>
#include <base/utility/DeltaTime.h>
#include "State/PlayerStateJump.h"
#include "State/PlayerStateAir.h"
#include <numbers>
#include <3d/Primitive/PrimitiveLineDrawer.h>
#include "State/Attack/PlayerStateAttack.h"
#include <scene/Transition/TransitionManager.h>
#include "State/PlayerStateDeath.h"
#include "State/PlayerStateClear.h"
#include "Controller/PlayerInput.h"
#include "2d/SpriteManager.h"

#include "../../../../Engine/input/Input.h"

Player::Player(std::string objectNama) : Object3d(objectNama)
{
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

void Player::Initialize()
{
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

	weapon_->AddRenderer(RendererManager::GetInstance()->FindRender("PlayerWeapon"));

	weapon_->AddCollider(CollisionManager::GetInstance()->FindCollider("WeaponCollider"));

	weapon_->Initialize();

	weapon_->GetWorldTransform()->SetParent(GetWorldTransform());

	scoreManager = std::make_unique<StylishScoreManager>();

	weapon_->SetScoreManager(scoreManager.get());

	reticle_ = SpriteManager::GetInstance()->CreateSprite(SpriteLayer::Game, "reticle", "reticle.png");
	reticle_->SetSize({ 32.0f, 32.0f });
	reticle_->SetAnchorPoint({ 0.5f, 0.5f });

	hitStop_ = std::make_unique<HitStop>();
}

void Player::Update(float deltaTime)
{
	hitStop_->Update(deltaTime);
	float dt = deltaTime * hitStop_->GetTimeScale();

	// Rキーを押したら死亡演出が流れる ← デバッグ用
	if (Input::GetInstance()->TriggerKey(DIK_R)) {
		ChangeState("Death");
	}

	scoreManager->Update();

	LockOn();

 	weapon_->Update(dt);

	if (!combat_->IsAttacking()) {
		stateMachine_->UpdateCurrentState(*this, dt);
	}
	combat_->Update(dt);

	for (auto& cmd : input_->GetCommands()) {
		ExecuteCommand(cmd);
	}

	GetWorldTransform()->GetTranslation() += velocity_ * dt;
	velocity_ += acceleration_ * dt;

	Object3d::Update(dt);

	// 接地フラグを毎フレーム切っておく
	onGround_ = false;

	if (lockOn_->IsLockOn()) {
		reticle_->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
	} else {
		reticle_->SetColor({ 1.0f, 1.0f, 1.0f, 0.0f });
	}

	if (lockOn_->IsLockOn()) {
		reticle_->SetPosition(CameraManager::GetInstance()->GetCurrentCamera()->WorldToScreen(lockOn_->GetCurrentTarget()->GetWorldPosition(), 1280, 720));
		reticle_->Update();
	}
}

void Player::Draw()
{
	weapon_->Draw();
	Object3d::Draw();

}

void Player::DrawEffect()
{

	combat_->Draw();
}

#ifdef _DEBUG
void Player::DebugGui()
{
	stateMachine_->DebugGui();
	ImGui::Begin("Player");
	Object3d::DebugGui();
	ImGui::End();

	//ImGui::Begin("Weapon");
	//weapon_->DebugGui();
	//ImGui::End();
}

#endif // _DEBUG

void Player::ChangeState(const std::string& stateName)
{
	stateMachine_->ChangeState(*this, stateName);
}

void Player::ExecuteCommand(const PlayerCommand& command)
{
	// ステートの更新
	if (!combat_->IsAttacking()) {
		stateMachine_->ExecuteCommand(*this, command);
	} 
	combat_->ExecuteCommand(command);
}

void Player::Move(float deltaTime)
{
	BaseCamera* camera = CameraManager::GetInstance()->GetActiveCamera();
	if (!camera) return;
	// 入力のcontext
	const PlayerInputContext& context = input_->GetContext();

	// Yは維持する
	velocity_ = { 0.0f, velocity_.y, 0.0f };
	// 入力方向をローカル（プレイヤーから見た）方向で作成
	Vector3 inputDir = { 0.0f, 0.0f, 0.0f };

	if (context.isMove) {
		inputDir = { context.move.x, 0.0f, context.move.y };

		if (Length(inputDir) > 0.01f) {
			inputDir = Normalize(inputDir);

			// カメラ基準で移動させる
			Vector3 camForward = camera->GetForward();
			camForward.y = 0.0f;
			camForward = Normalize(camForward);

			Vector3 camRight = camera->GetRight();
			camRight.y = 0.0f;
			camRight = Normalize(camRight);

			Vector3 moveDir = camRight * inputDir.x + camForward * inputDir.z;
			moveDir = Normalize(moveDir);

			float velocityY = velocity_.y;
			velocity_ = moveDir * 10.0f;
			velocity_.y = velocityY;

			// 向き更新
			if (!lockOn_->IsLockOn()) {
				if (Length(moveDir) > 0.001f) {
					moveDir.x *= -1.0f;

					Quaternion targetRot = LookRotation(moveDir);

					Quaternion& currentRot = GetWorldTransform()->GetRotation();

					// 補間
					currentRot = Slerp(currentRot, targetRot, rotateSpeed_ * deltaTime);
				}
			}
		}
	}
}

void Player::LockOn()
{
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

void Player::OnCollisionEnter(BaseCollider* other)
{
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
		} else if (outNormal.x == -1.0f) {
			// 左に当たってる
			GetWorldTransform()->GetTranslation().x = blockCollider->GetMin().x - playerOffset;
		} else if (outNormal.y == 1.0f) {
			// 上に当たってる
			GetWorldTransform()->GetTranslation().y = blockCollider->GetMax().y + playerOffset;
			GetWorldTransform()->GetTranslation().y -= 0.1f;
			//velocity_.y = 0.0f;
			onGround_ = true;
		} else if (outNormal.y == -1.0f) {
			// 下に当たってる
			GetWorldTransform()->GetTranslation().y = blockCollider->GetMin().y - playerOffset;
			//velocity_ *= -1.0f;
		} else if (outNormal.z == 1.0f) {
			// 奥に当たってる
			GetWorldTransform()->GetTranslation().z = blockCollider->GetMax().z + playerOffset;
		} else if (outNormal.z == -1.0f) {
			// 手前に当たってる
			GetWorldTransform()->GetTranslation().z = blockCollider->GetMin().z - playerOffset;
		}
	}
}

void Player::OnCollisionStay(BaseCollider* other)
{
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
		} else if (outNormal.x == -1.0f) {
			// 左に当たってる
			GetWorldTransform()->GetTranslation().x = blockCollider->GetMin().x - playerOffset;
		} else if (outNormal.y == 1.0f) {
			// 上に当たってる
			GetWorldTransform()->GetTranslation().y = blockCollider->GetMax().y + playerOffset;
			GetWorldTransform()->GetTranslation().y -= 0.1f;
			//velocity_.y = 0.0f;
			onGround_ = true;
		} else if (outNormal.y == -1.0f) {
			// 下に当たってる
			GetWorldTransform()->GetTranslation().y = blockCollider->GetMin().y - playerOffset;
			//velocity_ *= -1.0f;
		} else if (outNormal.z == 1.0f) {
			// 奥に当たってる
			GetWorldTransform()->GetTranslation().z = blockCollider->GetMax().z + playerOffset;
		} else if (outNormal.z == -1.0f) {
			// 手前に当たってる
			GetWorldTransform()->GetTranslation().z = blockCollider->GetMin().z - playerOffset;
		}
	}
}

void Player::OnCollisionExit(BaseCollider* other)
{
	other;
}
