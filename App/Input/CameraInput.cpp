#include "CameraInput.h"
#include "Input/input.h" 

void CameraInput::Initialize(Input* input)
{
	input_ = input;
}

void CameraInput::Updata()
{
	context_.stickDirection = { 0.0f, 0.0f };
	context_.stickMagnitude = 0.0f;

	if (input_->IsConnected()) {
		// コントローラーが接続されている場合の入力処理
			// XInputの右スティックのX軸の値を取得
		context_.stickDirection.x = input_->GetRightStickX();
		context_.stickDirection.y = input_->GetRightStickY();
		// スティックの傾きの大きさを計算
		context_.stickMagnitude = sqrtf(context_.stickDirection.x * context_.stickDirection.x + context_.stickDirection.y * context_.stickDirection.y);
	}
	else {
		// キーボードが使用されている場合の入力処理
		if (input_->PushKey(DIK_LEFT)) {
			context_.stickDirection.x -= 1.0f;
		}
		if (input_->PushKey(DIK_RIGHT)) {
			context_.stickDirection.x += 1.0f;
		}
		if (input_->PushKey(DIK_UP)) {
			context_.stickDirection.y -= 1.0f;
		}
		if (input_->PushKey(DIK_DOWN)) {
			context_.stickDirection.y += 1.0f;
		}
		// スティックの傾きの大きさを計算
		context_.stickMagnitude = sqrtf(context_.stickDirection.x * context_.stickDirection.x + context_.stickDirection.y * context_.stickDirection.y);
	}
}
