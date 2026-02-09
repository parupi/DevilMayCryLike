#include "PlayerInput.h"
#include "input/Input.h"

void PlayerInput::Initialize(Input* input)
{
	input_ = input;
}

void PlayerInput::Update()
{
	// 現在の状態を更新
	context_.move = {0.0f, 0.0f};
	context_.isMove = false;

	if (input_->IsConnected()) {
		if (input_->GetLeftStickX() != 0.0f || input_->GetLeftStickY() != 0.0f) {
			context_.move = { input_->GetLeftStickX(), input_->GetLeftStickY() };
			context_.isMove = true;
		}
	} else {
		if (input_->PushKey(DIK_W)) {
			context_.move = { 0.0f, 1.0f };
			context_.isMove = true;
		} else if (input_->PushKey(DIK_A)) {
			context_.move = { -1.0f, 0.0f };
			context_.isMove = true;
		} else if (input_->PushKey(DIK_S)) {
			context_.move = { 0.0f, -1.0f };
			context_.isMove = true;
		} else if (input_->PushKey(DIK_D)) {
			context_.move = { 1.0f, 0.0f };
			context_.isMove = true;
		}
	}

	// 毎フレーム削除する
	commands_.clear();


	if (input_->IsConnected()) {
		if (input_->GetLeftStickX() != 0.0f || input_->GetLeftStickY() != 0.0f) {
			PlayerCommand command{};
			command.action = PlayerAction::Move;
			command.stickDir = { input_->GetLeftStickX(), input_->GetLeftStickY() };
			commands_.push_back(command);
		}
	} else {
		if (input_->PushKey(DIK_W)) {
			PlayerCommand command{};
			command.action = PlayerAction::Move;
			command.stickDir = { 0.0f, 1.0f };
			commands_.push_back(command);
		} else if (input_->PushKey(DIK_A)) {
			PlayerCommand command{};
			command.action = PlayerAction::Move;
			command.stickDir = { -1.0f, 0.0f };
			commands_.push_back(command);
		} else if (input_->PushKey(DIK_S)) {
			PlayerCommand command{};
			command.action = PlayerAction::Move;
			command.stickDir = { 0.0f, -1.0f };
			commands_.push_back(command);
		} else if (input_->PushKey(DIK_D)) {
			PlayerCommand command{};
			command.action = PlayerAction::Move;
			command.stickDir = { 1.0f, 0.0f };
			commands_.push_back(command);
		}
	}

	if (input_->TriggerKey(DIK_SPACE) || input_->TriggerButton(ButtonA)) {
		PlayerCommand command{};
		command.action = PlayerAction::Jump;
		command.stickDir = { 0.0f, 0.0f };
		commands_.push_back(command);
	}

	if (input_->IsConnected()) {
		if (input_->TriggerButton(ButtonY)) {
			PlayerCommand command{};
			command.action = PlayerAction::Attack;
			command.stickDir = { input_->GetLeftStickX(), input_->GetLeftStickY() };
			commands_.push_back(command);
		}
	} else {
		if (input_->TriggerKey(DIK_J)) {
			PlayerCommand command{};
			command.action = PlayerAction::Attack;
			command.stickDir = { 0.0f, 0.0f };
			commands_.push_back(command);
		}
	}
}
