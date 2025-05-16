#include "Player.h"

void Player::Initialize()
{
	object_ = std::make_unique<Object3d>();
	object_->Initialize("float_body.obj");

}

void Player::Update()
{

	Vector3 translate = object_->GetWorldTransform()->GetTranslation();

	if (input->PushKey(DIK_W)) {
		translate.z += 0.2f;
	}
	if (input->PushKey(DIK_A)) {
		translate.x -= 0.2f;
	}
	if (input->PushKey(DIK_S)) {
		translate.z -= 0.2f;
	}
	if (input->PushKey(DIK_D)) {
		translate.x += 0.2f;
	}

	if (input->TriggerKey(DIK_SPACE)) {
		velocity_.y = 1.0f;
	}
	velocity_.y -= 0.05f;

	translate += velocity_;

	if (translate.y < 0.0f) {
		translate.y = 0.0f;
		velocity_.y = 0.0f;
	}

	object_->GetWorldTransform()->SetTranslation(translate);


	object_->Update();
}

void Player::Draw()
{
	object_->Draw();
}
