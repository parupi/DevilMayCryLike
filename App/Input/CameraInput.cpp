#include "CameraInput.h"
#include "Input/Input.h"

void CameraInput::Initialize(Input* input)
{
	input_ = input;
}

void CameraInput::Update()
{
	context_.stickDirection = { 0.0f, 0.0f };
	context_.stickMagnitude = 0.0f;

	if (input_->IsConnected()) {
		// コントローラーが接続されている場合の入力処理
			// XInputの右スティックのX軸の値を取得
		context_.stickDirection.x = input_->GetRightStickX();
		context_.stickDirection.y = input_->GetRightStickY();
	}

#ifdef _DEBUG
	// デバッグ用：コントローラー接続時でも矢印キーでカメラを回転できるようにする
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
#else
	if (!input_->IsConnected()) {
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
	}
#endif

	// スティックの傾きの大きさを計算
	context_.stickMagnitude = sqrtf(context_.stickDirection.x * context_.stickDirection.x + context_.stickDirection.y * context_.stickDirection.y);
}
