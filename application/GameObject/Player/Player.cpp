#include "Player.h"

void Player::Initialize()
{
	objectBody_ = std::make_unique<Object3d>();
	objectBody_->Initialize("PlayerBody");

	objectHead_ = std::make_unique<Object3d>();
	objectHead_->Initialize("PlayerHead");

	objectLeftArm_ = std::make_unique<Object3d>();
	objectLeftArm_->Initialize("PlayerLeftArm");

	objectRightArm_ = std::make_unique<Object3d>();
	objectRightArm_->Initialize("PlayerRightArm");

	objectLeftArm_->GetWorldTransform()->SetParent(objectBody_->GetWorldTransform());
	objectRightArm_->GetWorldTransform()->SetParent(objectBody_->GetWorldTransform());
	objectHead_->GetWorldTransform()->SetParent(objectBody_->GetWorldTransform());


	objectHead_->GetWorldTransform()->SetTranslation({});
	objectLeftArm_->GetWorldTransform()->SetTranslation({ -0.6f, 0.7f, 0.0f });
	objectRightArm_->GetWorldTransform()->SetTranslation({ 0.6f, 0.7f, 0.0f });
}

void Player::Update()
{
	Vector3 translate = objectBody_->GetWorldTransform()->GetTranslation();
	Quaternion rotation = objectBody_->GetWorldTransform()->GetRotation();

	if (input->PushKey(DIK_W)) {
		velocity_.z = 0.2f;
	}
	else if (input->PushKey(DIK_A)) {
		velocity_.x = -0.2f;
	}
	else if (input->PushKey(DIK_S)) {
		velocity_.z = -0.2f;
	}
	else if (input->PushKey(DIK_D)) {
		velocity_.x = 0.2f;
	} else {
		velocity_.x = 0.0f;
		velocity_.z = 0.0f;
	}

	// ジャンプのトリガー
	if (input->TriggerKey(DIK_SPACE)) {
		velocity_.y = 1.0f;
	}
	velocity_.y -= 0.05f;

	translate += velocity_;

	// 地面より下にいたら止まる
	if (translate.y < 0.0f) {
		translate.y = 0.0f;
		velocity_.y = 0.0f;
	}

	// 体を進行方向に向ける
	if (Length(velocity_) > 0.01f) {
		Vector3 forward = Normalize(velocity_);
		forward.y = 0.0f;
		Vector3 defaultForward = { 0.0f, 0.0f, 1.0f }; // モデルのデフォルト前方がZ+想定
		rotation = FromToRotation(defaultForward, forward);
	}

	// 攻撃のトリガー
	if (input->TriggerKey(DIK_J)) {
		isAttack_ = true;
		timeAttackCurrent_ = 0.0f;
		startDir_ = {0.01f, 0.01f, 0.01f};
		endDir_ = {-3.0f, 0.0f, 0.0f};
	}

	Quaternion newRot;
	// 攻撃中の動き
	if (isAttack_) {
		timeAttackCurrent_ += 1.0f / timeAttackMax_ * (1.0f / 60.0f);

		if (timeAttackCurrent_ < 0.5f) {
			startDir_ = { 0.01f, 0.01f, 0.01f };
			endDir_ = { -3.0f, 0.01f, 0.01f };
		} else if (timeAttackCurrent_ < 1.0f) {
			startDir_ = { -3.0f, 0.01f, 0.01f };
			endDir_ = { 0.01f, -0.51f, -0.01f };
		}

		Quaternion targetRot = FromToRotation(startDir_, Normalize(endDir_));

		Quaternion currentRot = objectRightArm_->GetWorldTransform()->GetRotation(); // 腕の現在のクォータニオン
		newRot = Slerp(currentRot, targetRot, timeAttackCurrent_); // 0.1fは補間速度

		if (timeAttackCurrent_ >= 1.0f) {
			isAttack_ = false;
		}
	}


	// 移動回転を適用
	objectBody_->GetWorldTransform()->SetTranslation(translate);
	objectBody_->GetWorldTransform()->SetRotation(rotation);
	objectRightArm_->GetWorldTransform()->SetRotation(newRot);

	//Quaternion rot = objectRightArm_->GetWorldTransform()->GetRotation();

	//ImGui::Begin("Player");
	//ImGui::DragFloat4("Rotation", &rot.x, 0.01f);
	//ImGui::End();

	//objectRightArm_->GetWorldTransform()->SetRotation(rot);


	objectBody_->Update();
	objectHead_->Update();
	objectLeftArm_->Update();
	objectRightArm_->Update();
}

void Player::Draw()
{
	objectBody_->Draw();
	objectHead_->Draw();
	objectLeftArm_->Draw();
	objectRightArm_->Draw();
}
