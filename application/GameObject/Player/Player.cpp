#include "Player.h"
#include "State/PlayerStateIdle.h"
#include "State/PlayerStateMove.h"
#include <Renderer/RendererManager.h>
#include <Renderer/PrimitiveRenderer.h>
#include <Collider/AABBCollider.h>
#include <Collider/CollisionManager.h>
#include <utility/DeltaTime.h>
#include "State/PlayerStateJump.h"
#include "State/PlayerStateAir.h"
#include <numbers>
#include <Primitive/PrimitiveLineDrawer.h>
#include "State/Attack/PlayerStateAttack1.h"


Player::Player(std::string objectNama) : Object3d(objectNama)
{
	Object3d::Initialize();

	// レンダラーの生成
	//RendererManager::GetInstance()->AddRenderer(std::make_unique<ModelRenderer>("PlayerBody", "PlayerBody"));
	RendererManager::GetInstance()->AddRenderer(std::make_unique<ModelRenderer>("PlayerHead", "PlayerHead"));

	//AddRenderer(RendererManager::GetInstance()->FindRender("PlayerBody"));
	AddRenderer(RendererManager::GetInstance()->FindRender("PlayerHead"));

	states_["Idle"] = std::make_unique<PlayerStateIdle>();
	states_["Move"] = std::make_unique<PlayerStateMove>();
	states_["Jump"] = std::make_unique<PlayerStateJump>();
	states_["Air"] = std::make_unique<PlayerStateAir>();
	states_["Attack1"] = std::make_unique<PlayerStateAttack1>("Attack1");
	currentState_ = states_["Idle"].get();
}

void Player::Initialize()
{
	static_cast<AABBCollider*>(GetCollider(name))->GetColliderData().offsetMax *= 0.5f;
	static_cast<AABBCollider*>(GetCollider(name))->GetColliderData().offsetMin *= 0.5f;
	// 武器のレンダラー生成
	RendererManager::GetInstance()->AddRenderer(std::make_unique<ModelRenderer>("PlayerWeapon", "weapon"));
	// 武器用のコライダー生成
	CollisionManager::GetInstance()->AddCollider(std::make_unique<AABBCollider>("WeaponCollider"));
	// 武器を生成
	weapon_ = std::make_unique<PlayerWeapon>("PlayerWeapon");

	weapon_->AddRenderer(RendererManager::GetInstance()->FindRender("PlayerWeapon"));

	weapon_->AddCollider(CollisionManager::GetInstance()->FindCollider("WeaponCollider"));

	weapon_->Initialize();

	weapon_->GetWorldTransform()->SetParent(GetWorldTransform());

	GetRenderer("PlayerHead")->GetWorldTransform()->GetTranslation().y -= 1.5f;
}

void Player::Update()
{
	if (currentState_) {
		currentState_->Update(*this);
	}

	GetWorldTransform()->GetTranslation() += velocity_ * DeltaTime::GetDeltaTime();
	velocity_ += acceleration_ * DeltaTime::GetDeltaTime();

	// 毎フレーム切っておく
	onGround_ = false;

	weapon_->Update();
	Object3d::Update();

	// ImGuiによるエディターを描画
	for (auto& [name, state] : states_) {
		PlayerStateAttackBase* attackState = dynamic_cast<PlayerStateAttackBase*>(state.get());
		if (attackState) {
			DrawAttackDataEditor(attackState);
			attackState->UpdateAttackData();
		}
	}
}

void Player::Draw()
{
	weapon_->Draw();
	Object3d::Draw();


	for (auto& [name, state] : states_) {
		PlayerStateAttackBase* attackState = dynamic_cast<PlayerStateAttackBase*>(state.get());
		if (attackState) {
			attackState->DrawControlPoints(*this);
		}
	}
}

void Player::DrawEffect()
{

}

void Player::DrawAttackDataEditor(PlayerStateAttackBase* attack)
{
	const char* attackName = attack->name.c_str();
	ImGui::Begin(attackName);

	int32_t& pointCount = gv->GetValueRef<int32_t>(attackName, "PointCount");

	for (int32_t i = 0; i < pointCount; ++i) {
		Vector3& point = gv->GetValueRef<Vector3>(attackName, "ControlPoint_" + std::to_string(i));
		ImGui::DragFloat3(("P" + std::to_string(i)).c_str(), &point.x, 0.01f);
	}

	if (ImGui::Button("Add Point")) {
		gv->AddItem(attackName, "ControlPoint_" + std::to_string(pointCount), Vector3{});
		++pointCount;
	}

	if (ImGui::Button("Remove Last") && pointCount > 0) {
		--pointCount;
		gv->RemoveItem(attackName, "ControlPoint_" + std::to_string(pointCount));
	}
	ImGui::Separator();

	// 移動系
	ImGui::DragFloat("Move Speed", &gv->GetValueRef<float>(attackName, "MoveSpeed"), 0.01f);
	ImGui::DragFloat("KnockBack Speed", &gv->GetValueRef<float>(attackName, "KnockBackSpeed"), 0.01f);

	ImGui::Separator();

	// タイマー系
	ImGui::DragFloat("Total Duration", &gv->GetValueRef<float>(attackName, "TotalDuration"), 0.01f);
	ImGui::DragFloat("Pre Delay", &gv->GetValueRef<float>(attackName, "PreDelay"), 0.01f);
	ImGui::DragFloat("Attack Duration", &gv->GetValueRef<float>(attackName, "AttackDuration"), 0.01f);
	ImGui::DragFloat("Post Delay", &gv->GetValueRef<float>(attackName, "PostDelay"), 0.01f);
	ImGui::DragFloat("Next Attack Delay", &gv->GetValueRef<float>(attackName, "NextAttackDelay"), 0.01f);

	ImGui::Separator();

	// その他
	ImGui::Checkbox("Draw Debug Control Points", &gv->GetValueRef<bool>(attackName, "DrawDebugControlPoints"));

	ImGui::DragFloat("Damage", &gv->GetValueRef<float>(attackName, "Damage"), 0.01f);

	ImGui::Separator();

	if (ImGui::Button("Save")) {
		gv->SaveFile(attackName);
		std::string message = std::format("{}.json saved.", attackName);
		MessageBoxA(nullptr, message.c_str(), "GlobalVariables", 0);
	}
	ImGui::End();
}



#ifdef _DEBUG
void Player::DebugGui()
{

	ImGui::Begin("Object");
	Object3d::DebugGui();
	ImGui::End();

	ImGui::Begin("Weapon");
	weapon_->DebugGui();
	ImGui::End();

}

#endif // _DEBUG

void Player::ChangeState(const std::string& stateName)
{
	currentState_->Exit(*this);
	auto it = states_.find(stateName);
	if (it != states_.end()) {
		currentState_ = it->second.get();
		currentState_->Enter(*this);
	}
}

void Player::Move()
{
	Camera* camera = CameraManager::GetInstance()->GetActiveCamera();
	if (!camera) return;

	// 各フレームでまず速度をゼロに初期化
	velocity_ = { 0.0f, velocity_.y, 0.0f };
	// 入力方向をローカル（プレイヤーから見た）方向で作成
	Vector3 inputDir = { 0.0f, 0.0f, 0.0f };
	if (Input::GetInstance()->PushKey(DIK_W)) inputDir.z += 1.0f;
	if (Input::GetInstance()->PushKey(DIK_S)) inputDir.z -= 1.0f;
	if (Input::GetInstance()->PushKey(DIK_D)) inputDir.x += 1.0f;
	if (Input::GetInstance()->PushKey(DIK_A)) inputDir.x -= 1.0f;

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

		// プレイヤーの向きも更新
		if (Length(moveDir) > 0.001f) {
			moveDir.x *= -1.0f;
			Quaternion lookRot = LookRotation(moveDir);

			GetWorldTransform()->GetRotation() = lookRot;
		}
	}
}

void Player::OnCollisionEnter(BaseCollider* other)
{
	if (other->category_ == CollisionCategory::Ground) {

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
	if (other->category_ == CollisionCategory::Ground) {

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
}
