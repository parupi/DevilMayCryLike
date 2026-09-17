#include "TimeManager.h"
#include "DeltaTime.h"
#include <algorithm>
// 表示は Engine/Editor/Windows/TimeWindow.cpp が担当する

float TimeManager::realDelta_ = 0.0f;
float TimeManager::gameTimeScale_ = 1.0f;
float TimeManager::vfxBias_ = 0.3f;

void TimeManager::Update()
{
	realDelta_ = DeltaTime::GetDeltaTime();

	// このフレームの要求を受け付ける前に通常速度へ戻しておく。
	// 要求は毎フレーム申告される前提なので、止める側が消えれば自動的に1.0へ復帰する。
	gameTimeScale_ = 1.0f;
}

void TimeManager::RequestGameTimeScale(float scale)
{
	gameTimeScale_ = std::min(gameTimeScale_, std::clamp(scale, 0.0f, 1.0f));
}

float TimeManager::GetVFXTimeScale()
{
	// gameTimeScale_ と 1.0 の間を vfxBias_ で補間する。
	// ゲームが完全停止(0.0)していても vfxBias_ の割合だけVFXは進む。
	return gameTimeScale_ + (1.0f - gameTimeScale_) * vfxBias_;
}

void TimeManager::SetVFXBias(float bias)
{
	vfxBias_ = std::clamp(bias, 0.0f, 1.0f);
}
