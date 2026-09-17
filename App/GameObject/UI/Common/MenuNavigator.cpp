#include "MenuNavigator.h"
#include <Input/Input.h>
#include <Utility/DeltaTime.h>

void MenuNavigator::Update()
{
	const Input& input = Input::GetInstance();
	// メニューはゲーム内の時間スケールに関係なく動かしたいので実時間で数える
	const float deltaTime = DeltaTime::GetUnscaledDeltaTime();

	bool up = input.PushKey(DIK_W) || input.PushKey(DIK_UP);
	bool down = input.PushKey(DIK_S) || input.PushKey(DIK_DOWN);
	bool left = input.PushKey(DIK_A) || input.PushKey(DIK_LEFT);
	bool right = input.PushKey(DIK_D) || input.PushKey(DIK_RIGHT);

	decide_ = input.TriggerKey(DIK_SPACE) || input.TriggerKey(DIK_RETURN);
	cancel_ = input.TriggerKey(DIK_ESCAPE) || input.TriggerKey(DIK_BACK);

	if (input.IsConnected()) {
		// PushButton は wButtons とのビット比較なので、PadNumber に無い十字キーも直接渡せる
		up = up || input.PushButton(XINPUT_GAMEPAD_DPAD_UP) || input.GetLeftStickY() > kStickThreshold;
		down = down || input.PushButton(XINPUT_GAMEPAD_DPAD_DOWN) || input.GetLeftStickY() < -kStickThreshold;
		left = left || input.PushButton(XINPUT_GAMEPAD_DPAD_LEFT) || input.GetLeftStickX() < -kStickThreshold;
		right = right || input.PushButton(XINPUT_GAMEPAD_DPAD_RIGHT) || input.GetLeftStickX() > kStickThreshold;

		decide_ = decide_ || input.TriggerButton(PadNumber::ButtonA);
		cancel_ = cancel_ || input.TriggerButton(PadNumber::ButtonB);
	}

	UpdateAxis(up_, up, deltaTime);
	UpdateAxis(down_, down, deltaTime);
	UpdateAxis(left_, left, deltaTime);
	UpdateAxis(right_, right, deltaTime);
}

void MenuNavigator::UpdateAxis(Axis& axis, bool pressed, float deltaTime)
{
	if (!pressed) {
		axis.pressed = false;
		axis.triggered = false;
		axis.repeatTimer = 0.0f;
		return;
	}

	// 倒した瞬間は必ず1回ぶん動かし、そのあと少し待ってからリピートに入る
	if (!axis.pressed) {
		axis.pressed = true;
		axis.triggered = true;
		axis.repeatTimer = kRepeatDelay;
		return;
	}

	axis.repeatTimer -= deltaTime;
	if (axis.repeatTimer <= 0.0f) {
		axis.triggered = true;
		axis.repeatTimer = kRepeatInterval;
	} else {
		axis.triggered = false;
	}
}
