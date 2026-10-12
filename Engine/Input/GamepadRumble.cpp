#include "GamepadRumble.h"

#include <Windows.h>
#include <Xinput.h>

#include <algorithm>

namespace {
	// 今のところパッドは1台目だけを見る（Input と同じ）
	constexpr DWORD kPadIndex = 0;

	// ゲームのウィンドウが前面にあるか。裏に回っている間に震え続けないようにする
	bool IsOwnWindowForeground() {
		HWND foreground = GetForegroundWindow();
		if (!foreground) return false;
		DWORD processId = 0;
		GetWindowThreadProcessId(foreground, &processId);
		return processId == GetCurrentProcessId();
	}

	// 0〜1 を XInput のモーター値（0〜65535）へ
	WORD ToMotor(float value) {
		return static_cast<WORD>(std::clamp(value, 0.0f, 1.0f) * 65535.0f + 0.5f);
	}
}

GamepadRumble& GamepadRumble::GetInstance()
{
	static GamepadRumble instance;
	return instance;
}

void GamepadRumble::Play(const RumbleRequest& request)
{
	if (request.duration <= 0.0f) return;
	if (request.low <= 0.0f && request.high <= 0.0f) return;
	actives_.push_back({ request, 0.0f });
}

void GamepadRumble::SetMasterScale(float scale)
{
	masterScale_ = std::clamp(scale, 0.0f, 1.0f);
}

void GamepadRumble::Update(float realDeltaTime)
{
	float low = 0.0f;
	float high = 0.0f;

	for (Active& active : actives_) {
		active.elapsed += realDeltaTime;
		const RumbleRequest& request = active.request;
		if (active.elapsed >= request.duration) continue;

		// 終わりぎわだけ直線で弱める
		const float fadeStart = request.duration * (1.0f - std::clamp(request.fadeRatio, 0.0f, 1.0f));
		float envelope = 1.0f;
		if (active.elapsed > fadeStart && request.duration > fadeStart) {
			envelope = 1.0f - (active.elapsed - fadeStart) / (request.duration - fadeStart);
		}
		low = (std::max)(low, request.low * envelope);
		high = (std::max)(high, request.high * envelope);
	}
	std::erase_if(actives_, [](const Active& active) { return active.elapsed >= active.request.duration; });

	if (!IsOwnWindowForeground()) {
		low = 0.0f;
		high = 0.0f;
	}
	Send(low * masterScale_, high * masterScale_);
}

void GamepadRumble::StopAll()
{
	actives_.clear();
	Send(0.0f, 0.0f);
}

void GamepadRumble::Send(float low, float high)
{
	// 毎フレーム同じ値を送らない（パッドが無いときも無駄に呼ばない）
	if (low == sentLow_ && high == sentHigh_) return;
	sentLow_ = low;
	sentHigh_ = high;

	XINPUT_VIBRATION vibration{};
	vibration.wLeftMotorSpeed = ToMotor(low);
	vibration.wRightMotorSpeed = ToMotor(high);
	XInputSetState(kPadIndex, &vibration);
}
