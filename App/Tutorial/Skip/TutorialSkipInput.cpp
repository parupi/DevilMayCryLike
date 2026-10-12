#include "TutorialSkipInput.h"

#include "Input/Input.h"

#include <algorithm>

bool TutorialSkipInput::IsPressed()
{
	Input& input = Input::GetInstance();
	return input.PushButton(ButtonStart) || input.PushKey(DIK_M);
}

void TutorialSkipInput::Reset()
{
	holding_ = false;
	holdTime_ = 0.0f;
	// 押したまま戻ってきた場合に備えて、一度離すまで数えない
	waitRelease_ = true;
	prevPressed_ = true;
}

TutorialSkipInput::Result TutorialSkipInput::Update(float deltaTime, bool enabled)
{
	// 一度決まったら満タンのまま。シーン遷移の暗転中にゲージが減っていくと見た目がおかしい
	if (skipped_) return Result::None;

	const bool pressed = IsPressed();
	const bool triggered = pressed && !prevPressed_;
	prevPressed_ = pressed;

	if (!pressed) {
		waitRelease_ = false;
	}
	if (!enabled) {
		// 押していても数えない。再開したときに押しっぱなしのぶんが数えられないようにする
		waitRelease_ = waitRelease_ || pressed;
		holding_ = false;
		holdTime_ = 0.0f;
		progress_ = std::clamp(progress_ - kDrainSpeed * deltaTime, 0.0f, 1.0f);
		return Result::None;
	}

	if (triggered && !waitRelease_) {
		holding_ = true;
		holdTime_ = 0.0f;
	}

	if (holding_) {
		if (pressed) {
			holdTime_ += deltaTime;
			progress_ = std::clamp(holdTime_ / kHoldDuration, 0.0f, 1.0f);
			if (progress_ >= 1.0f) {
				holding_ = false;
				skipped_ = true;
				return Result::Skip;
			}
			return Result::None;
		}

		// 離した。短ければポーズ、長ければ「やめた」で何もしない
		holding_ = false;
		const bool tapped = holdTime_ <= kTapMaxDuration;
		holdTime_ = 0.0f;
		if (tapped) {
			return Result::Pause;
		}
		return Result::None;
	}

	progress_ = std::clamp(progress_ - kDrainSpeed * deltaTime, 0.0f, 1.0f);
	return Result::None;
}
