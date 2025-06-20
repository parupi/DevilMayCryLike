#include "Player.h"
#include "State/PlayerStateIdle.h"
#include "State/PlayerStateMove.h"
#include "State/PlayerStateAttack1.h"
#include <Renderer/RendererManager.h>
#include <Renderer/PrimitiveRenderer.h>
#include <Collider/AABBCollider.h>
#include <Collider/CollisionManager.h>
#include <utility/DeltaTime.h>
#include "State/PlayerStateJump.h"
#include "State/PlayerStateAir.h"
#include <numbers>

Player::Player(std::string objectNama) : Object3d(objectNama)
{
	Object3d::Initialize();

	// レンダラーの生成
	RendererManager::GetInstance()->AddRenderer(std::make_unique<ModelRenderer>("PlayerBody", "PlayerBody"));
	RendererManager::GetInstance()->AddRenderer(std::make_unique<ModelRenderer>("PlayerHead", "PlayerHead"));
	RendererManager::GetInstance()->AddRenderer(std::make_unique<ModelRenderer>("PlayerLeftArm", "PlayerLeftArm"));
	RendererManager::GetInstance()->AddRenderer(std::make_unique<ModelRenderer>("PlayerRightArm", "PlayerRightArm"));

	AddRenderer(RendererManager::GetInstance()->FindRender("PlayerBody"));
	AddRenderer(RendererManager::GetInstance()->FindRender("PlayerHead"));
	AddRenderer(RendererManager::GetInstance()->FindRender("PlayerLeftArm"));
	AddRenderer(RendererManager::GetInstance()->FindRender("PlayerRightArm"));

	states_["Idle"] = std::make_unique<PlayerStateIdle>();
	states_["Move"] = std::make_unique<PlayerStateMove>();
	states_["Jump"] = std::make_unique<PlayerStateJump>();
	states_["Air"] = std::make_unique<PlayerStateAir>();
	states_["Attack1"] = std::make_unique<PlayerStateAttack1>();
	currentState_ = states_["Idle"].get();
}

void Player::Initialize()
{
	GetRenderer("PlayerLeftArm")->GetWorldTransform()->GetTranslation() = { -0.4f, 0.8f, 0.0f };
	GetRenderer("PlayerRightArm")->GetWorldTransform()->GetTranslation() = { 0.4f, 0.8f, 0.0f };
	static_cast<AABBCollider*>(GetCollider(name))->GetColliderData().offsetMax *= 0.5f;
	static_cast<AABBCollider*>(GetCollider(name))->GetColliderData().offsetMin *= 0.5f;

	RendererManager::GetInstance()->AddRenderer(std::make_unique<ModelRenderer>("PlayerWeapon", "weapon"));

	CollisionManager::GetInstance()->AddCollider(std::make_unique<AABBCollider>("WeaponCollider"));

	weapon_ = std::make_unique<PlayerWeapon>("PlayerWeapon");

	weapon_->AddRenderer(RendererManager::GetInstance()->FindRender("PlayerWeapon"));

	weapon_->AddCollider(CollisionManager::GetInstance()->FindCollider("WeaponCollider"));

	weapon_->Initialize();

	weapon_->GetWorldTransform()->SetParent(GetRenderer("PlayerRightArm")->GetWorldTransform());
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

	ImGui::Begin("Player");
	ImGui::DragFloat3("Translate", &GetWorldTransform()->GetTranslation().x, 0.01f);
	ImGui::DragFloat3("Rotate", &GetWorldTransform()->GetRotation().x, 0.01f);
	ImGui::DragFloat3("Scale", &GetWorldTransform()->GetScale().x, 0.01f);
	ImGui::End();
}

void Player::Draw()
{
	weapon_->Draw();
	Object3d::Draw();
}

void Player::DrawEffect()
{

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
	// 各フレームでまず速度をゼロに初期化
	velocity_ = { 0.0f, velocity_.y, 0.0f };

	if (Input::GetInstance()->PushKey(DIK_W)) {
		velocity_.z += 1.0f;
	}
	if (Input::GetInstance()->PushKey(DIK_S)) {
		velocity_.z -= 1.0f;
	}
	if (Input::GetInstance()->PushKey(DIK_D)) {
		velocity_.x += 1.0f;
	}
	if (Input::GetInstance()->PushKey(DIK_A)) {
		velocity_.x -= 1.0f;
	}

	if (Length(velocity_) > 0.01f) {
		// y座標は正規化させないでおく
		float velocityY = velocity_.y;
		velocity_ = Normalize(velocity_) * 10.0f;
		velocity_.y = velocityY;

		Vector3 moveDirection = velocity_;
		moveDirection.y = 0.0f;
		moveDirection.z *= -1.0f;

		// moveDirection がゼロベクトルかチェック
		if (Length(moveDirection) > 0.0001f) {
			// Z+が前を向くように回転を取得
			Quaternion lookRot = LookRotation(moveDirection);

			// Z+を向くように補正 → -Zを前にしたいのでY軸180度回転
			Quaternion correction = MakeRotateAxisAngleQuaternion({ 0.0f, 1.0f, 0.0f }, static_cast<float>(std::numbers::pi));

			// Apply correction BEFORE LookRotation
			GetWorldTransform()->GetRotation() = correction * lookRot; // ← 順番重要（補正を先に掛ける）
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
