#include "PlayerInput.h"
#include "Input/Input.h"

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

	// 回避（RT / 左Shift）。押した瞬間だけ拾う。
	// RT はアナログ値でボタンビットを持たないので Input の専用窓口を使う
	if (input_->TriggerRightTrigger() || input_->TriggerKey(DIK_LSHIFT)) {
		PlayerCommand command{};
		command.action = PlayerAction::Dodge;
		command.stickDir = context_.move;
		commands_.push_back(command);
	}

	if (input_->IsConnected()) {
		if (input_->TriggerButton(ButtonY)) {
			PlayerCommand command{};
			command.action = PlayerAction::Attack;
			command.button = InputButton::Y;
			command.stickDir = { input_->GetLeftStickX(), input_->GetLeftStickY() };
			commands_.push_back(command);
		}
		if (input_->TriggerButton(ButtonX)) {
			PlayerCommand command{};
			command.action = PlayerAction::Attack;
			command.button = InputButton::X;
			command.stickDir = { input_->GetLeftStickX(), input_->GetLeftStickY() };
			commands_.push_back(command);
		}
	} else {
		// 攻撃の方向条件（ロックオン中に 前 / 後ろ + 攻撃 で出る突進・切り上げ）は、
		// パッドのスティックと同じく移動キーの向きで判定する。
		// {0,0} 固定にしていた頃は、キーボードでは方向付きの攻撃が一切出なかった
		if (input_->TriggerKey(DIK_J)) {
			PlayerCommand command{};
			command.action = PlayerAction::Attack;
			command.button = InputButton::Y;
			command.stickDir = context_.move;
			commands_.push_back(command);
		}
		if (input_->TriggerKey(DIK_K)) {
			PlayerCommand command{};
			command.action = PlayerAction::Attack;
			command.button = InputButton::X;
			command.stickDir = context_.move;
			commands_.push_back(command);
		}
	}
}

bool PlayerInput::IsAttackButtonHeld(InputButton button) const
{
	// 割り当ては Update と同じ（パッドが繋がっていればパッド、無ければ J / K）
	const bool isPadConnected = input_->IsConnected();
	const bool isHoldY = isPadConnected ? input_->PushButton(ButtonY) : input_->PushKey(DIK_J);
	const bool isHoldX = isPadConnected ? input_->PushButton(ButtonX) : input_->PushKey(DIK_K);

	switch (button) {
	case InputButton::X:
		return isHoldX;
	case InputButton::Y:
		return isHoldY;
	default:
		return isHoldX || isHoldY;
	}
}
