#include "PlayerInput.h"
#include "input/Input.h"

void PlayerInput::Initialize(Input* input)
{
	input_ = input;
}

void PlayerInput::Update()
{
	// 毎フレーム削除する
	commands_.clear();

	if (input_->TriggerKey(DIK_W)) {
		PlayerCommand command{};
		command.action = PlayerAction::Move;
		command.moveDir = { 0.0f, 0.0f, 1.0f };
		commands_.push_back(command);
	} else if (input_->TriggerKey(DIK_A)) {
		PlayerCommand command{};
		command.action = PlayerAction::Move;
		command.moveDir = { -1.0f, 0.0f, 0.0f };
		commands_.push_back(command);
	} else if (input_->TriggerKey(DIK_S)) {
		PlayerCommand command{};
		command.action = PlayerAction::Move;
		command.moveDir = { 0.0f, 0.0f, -1.0f };
		commands_.push_back(command);
	} else if (input_->TriggerKey(DIK_D)) {
		PlayerCommand command{};
		command.action = PlayerAction::Move;
		command.moveDir = { 1.0f, 0.0f, 0.0f };
		commands_.push_back(command);
	}

}
