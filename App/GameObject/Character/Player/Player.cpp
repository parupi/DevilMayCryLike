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

	hitStop_ = std::make_unique<HitStop>();

	titleWord_ = std::make_unique<Sprite>();
	titleWord_->Initialize("reticle.png");
	titleWord_->SetSize({ 32.0f, 32.0f });
	titleWord_->SetAnchorPoint({ 0.5f, 0.5f });

	// プレイヤーをカメラ側を向かせる
	GetWorldTransform()->GetRotation() = EulerDegree({ 0.0f, 180.0f, 0.0f });

	attackBranchUI_ = std::make_unique<AttackBranchUI>();
	attackBranchUI_->Initialize();
	attackBranchUI_->SetVisible(false);
}

void Player::Update(float deltaTime)
{
	// カメラ合切り替え中とフェード中は動かさない
	if (!TransitionManager::GetInstance()->IsFinished()/* || CameraManager::GetInstance()->IsTransition()*/ || isMenu_) {
		// 最初から更新しておきたいものはここに入れておく
		hitStop_->Update();
		scoreManager->Update();
		weapon_->Update(deltaTime);
		Object3d::Update(deltaTime);
		return;
	}

	for (auto& cmd : input_->GetCommands()) {
		ExecuteCommand(cmd);
	}

	// Rキーを押したら死亡演出が流れる
	if (Input::GetInstance()->TriggerKey(DIK_R)) {
		ChangeState("Death");
	}

	hitStop_->Update();
	scoreManager->Update();

	LockOn();


 	weapon_->Update(deltaTime);


	if (!combat_->IsAttacking()) {
		stateMachine_->UpdateCurrentState(*this, deltaTime);
	}
	combat_->Update(deltaTime);

	GetWorldTransform()->GetTranslation() += velocity_ * deltaTime;
	velocity_ += acceleration_ * deltaTime;

	Object3d::Update(deltaTime);
	// 毎フレーム切っておく
	onGround_ = false;

	if (lockOnEnemy_) {
		titleWord_->SetPosition(CameraManager::GetInstance()->GetActiveCamera()->WorldToScreen(lockOnEnemy_->GetWorldTransform()->GetTranslation(), 1280, 720));
		titleWord_->Update();
	}
}

void Player::Draw()
{
	weapon_->Draw();
	Object3d::Draw();

	combat_->Draw();
	SpriteManager::GetInstance()->DrawSet();
	attackBranchUI_->Draw();
}

void Player::DrawEffect()
{
	if (isLockOn_) {
		titleWord_->Draw();
	}
}

#ifdef _DEBUG
void Player::DebugGui()
{
	//ImGui::Begin("Player");
	//ImGui::Text("Current State: %s", currentStateName);
	//Object3d::DebugGui();
	//ImGui::End();

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
	switch (command.action) {
	case PlayerAction::Move:
		ChangeState("Move");
		break;
	case PlayerAction::Jump:
		ChangeState("Jump");
		break;
	}
}

void Player::Move()
{
	input = Input::GetInstance();
	BaseCamera* camera = CameraManager::GetInstance()->GetActiveCamera();
	if (!camera) return;

	// 各フレームでまず速度をゼロに初期化
	velocity_ = { 0.0f, velocity_.y, 0.0f };
	// 入力方向をローカル（プレイヤーから見た）方向で作成
	Vector3 inputDir = { 0.0f, 0.0f, 0.0f };

	if (input->IsConnected()) {
		if (input->GetLeftStickY() != 0.0f) {
			inputDir.z = input->GetLeftStickY();
		}
		if (input->GetLeftStickX() != 0.0f) {
			inputDir.x = input->GetLeftStickX();
		}
	} else {
		if (Input::GetInstance()->PushKey(DIK_W)) inputDir.z += 1.0f;
		if (Input::GetInstance()->PushKey(DIK_S)) inputDir.z -= 1.0f;
		if (Input::GetInstance()->PushKey(DIK_D)) inputDir.x += 1.0f;
		if (Input::GetInstance()->PushKey(DIK_A)) inputDir.x -= 1.0f;
	}

	if (Length(inputDir) > 0.01f) {
		inputDir = Normalize(inputDir);

		// カメラのforward/right（Y方向カット）
		Vector3 camForward = camera->GetForward(); // カメラの「向き」
		camForward.y = 0.0f;
		camForward = Normalize(camForward);

		Vector3 camRight = camera->GetRight(); // カメラの右
		camRight.y = 0.0f;
		camRight = Normalize(camRight);

		// 入力方向をカメラの向きに投影
		Vector3 moveDir = camRight * inputDir.x + camForward * inputDir.z;
		moveDir = Normalize(moveDir);

		// 移動速度に反映
		float velocityY = velocity_.y;
		velocity_ = moveDir * 10.0f;
		velocity_.y = velocityY;

		// ロックオンしていなければプレイヤーの向きも更新
		if (!isLockOn_) {
			if (Length(moveDir) > 0.001f) {
				moveDir.x *= -1.0f;
				Quaternion lookRot = LookRotation(moveDir);

				GetWorldTransform()->GetRotation() = lookRot;
			}
		}
	}
}

void Player::LockOn()
{
	enemies_.clear();
	std::vector<Object3d*> objects;
	objects = Object3dManager::GetInstance()->GetAllObject();
	for (auto& object : objects) {
		if (!object->name_.find("HellKaina")) {
			enemies_.push_back(static_cast<Enemy*>(object));
		}
	}

	if (input->TriggerButton(PadNumber::ButtonR) || input->TriggerKey(DIK_P)) {
		isLockOn_ = true;
		// 敵を設定
		float lowDistance = 300.0f;
		for (auto& enemy : enemies_) {

			float length = Length(enemy->GetWorldTransform()->GetTranslation() - GetWorldTransform()->GetTranslation());
			if (length < lowDistance) {
				lowDistance = length;
				lockOnEnemy_ = enemy;
			}
		}
	}

	if (!input->PushButton(PadNumber::ButtonR) && !input->PushKey(DIK_P)) {
		isLockOn_ = false;
	}

	if (isLockOn_) {
		if (lockOnEnemy_->IsAlive()) {
			Vector3 direction = Normalize(lockOnEnemy_->GetWorldTransform()->GetTranslation() - GetWorldTransform()->GetTranslation());
			if (Length(direction) > 0.001f) {
				direction.x *= -1.0f;
				Quaternion lookRot = LookRotation(direction);

				GetWorldTransform()->GetRotation() = lookRot;
			}
		} else {
			isLockOn_ = false;
			lockOnEnemy_ = nullptr;
		}
	}
	if (isLockOn_) {
		if (GetLockOnPos().x == 0.0f && GetLockOnPos().y == 0.0f && GetLockOnPos().z == 0.0f) {
			isLockOn_ = false;
			lockOnEnemy_ = nullptr;
		}
	}
}

void Player::Clear()
{
	isClear_ = true;
}

void Player::SetIntent(const MoveIntent& intent)
{
	//movement_->SetIntent(intent);
}

void Player::RequestAttack(AttackType id)
{
	combat_->RequestAttack(id);
}

const Vector3& Player::GetLockOnPos()
{
	if (lockOnEnemy_) {
		return lockOnEnemy_->GetWorldTransform()->GetTranslation();
	}
	return {0.0f, 0.0f, 0.0f};
}

AttackInputState Player::GetAttackInputState() const
{
	AttackInputState state{};

	// ボタン
	state.y = input->TriggerButton(PadNumber::ButtonY) || input->TriggerKey(DIK_J);
	state.rb = input->PushButton(PadNumber::ButtonR);

	// スティック方向（しきい値あり）
	float lx = input->GetLeftStickX();
	float ly = input->GetLeftStickY();

	constexpr float threshold = 0.5f;

	if (ly > threshold)       state.dir = StickDir::Up;
	else if (ly < -threshold) state.dir = StickDir::Down;
	else if (lx > threshold)  state.dir = StickDir::Right;
	else if (lx < -threshold) state.dir = StickDir::Left;
	else                      state.dir = StickDir::Neutral;

	state.isLockOn = isLockOn_;

	return state;
}

void Player::OnCollisionEnter(BaseCollider* other)
{
	//if (other->category_ == CollisionCategory::Ground || other->category_ == CollisionCategory::Enemy) {

	//	AABBCollider* playerCollider = static_cast<AABBCollider*>(GetCollider("Player"));

	//	AABBCollider* blockCollider = static_cast<AABBCollider*>(other);

	//	Vector3 outNormal = AABBCollider::CalculateCollisionNormal(playerCollider, blockCollider);

	//	Vector3 playerMin = playerCollider->GetMin();
	//	Vector3 playerMax = playerCollider->GetMax();

	//	float playerOffset = (playerMax.x - playerMin.x) * 0.5f;

	//	if (outNormal.x == 1.0f) {
	//		// 右に当たってる
	//		GetWorldTransform()->GetTranslation().x = blockCollider->GetMax().x + playerOffset;
	//	} else if (outNormal.x == -1.0f) {
	//		// 左に当たってる
	//		GetWorldTransform()->GetTranslation().x = blockCollider->GetMin().x - playerOffset;
	//	} else if (outNormal.y == 1.0f) {
	//		// 上に当たってる
	//		GetWorldTransform()->GetTranslation().y = blockCollider->GetMax().y + playerOffset;
	//		GetWorldTransform()->GetTranslation().y -= 0.1f;
	//		//velocity_.y = 0.0f;
	//		onGround_ = true;
	//	} else if (outNormal.y == -1.0f) {
	//		// 下に当たってる
	//		GetWorldTransform()->GetTranslation().y = blockCollider->GetMin().y - playerOffset;
	//		//velocity_ *= -1.0f;
	//	} else if (outNormal.z == 1.0f) {
	//		// 奥に当たってる
	//		GetWorldTransform()->GetTranslation().z = blockCollider->GetMax().z + playerOffset;
	//	} else if (outNormal.z == -1.0f) {
	//		// 手前に当たってる
	//		GetWorldTransform()->GetTranslation().z = blockCollider->GetMin().z - playerOffset;
	//	}
	//}
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

	//auto hits = collider_->GetHits();

	//auto resolveResult = collisionResolver_->Resolve(hits, velocity_);

	//GetWorldTransform()->GetTranslation() += resolveResult.positionOffset;
	//velocity_ += resolveResult.velocityOffset;

	//onGround_ = resolveResult.onGround;
}

void Player::OnCollisionExit(BaseCollider* other)
{
	other;
}
