#include "ClearSequence.h"
#include "Debugger/GlobalVariables.h"

#include <algorithm>
#include <type_traits>

namespace {

GlobalVariables& Global() { return GlobalVariables::GetInstance(); }

// 調整値の登録と読み出しを1か所に並べる（キーの書き間違いで片方だけ変わらないように）
template<typename Func>
void ForEachParam(ClearSequence::Params& p, Func&& func)
{
	func("LightBoostStart", p.lightBoostStart);
	func("LightBoostEnd", p.lightBoostEnd);
	func("LightBoostScale", p.lightBoostScale);
	func("StageClearStart", p.stageClearStart);
	func("StageClearDuration", p.stageClearDuration);
	func("ScoreStart", p.scoreStart);
	func("ScoreDuration", p.scoreDuration);
	func("RankStart", p.rankStart);
	func("RankDuration", p.rankDuration);
	func("RankEffectStart", p.rankEffectStart);
	func("MenuStart", p.menuStart);
	func("MenuDuration", p.menuDuration);
	func("CameraPosition", p.cameraPosition);
	func("CameraTarget", p.cameraTarget);
	func("CameraZoom", p.cameraZoom);
	func("RankZoomPunch", p.rankZoomPunch);
}

} // namespace

void ClearSequence::Initialize()
{
	GlobalVariables& gv = Global();
	gv.CreateGroup(kGroup);
	gv.LoadFile(kDirectory, kGroup);
	// ファイルに無い項目だけ既定値で足す（AddItem は既にある値を上書きしない）
	Params defaults{};
	ForEachParam(defaults, [&](const char* key, auto& value) { gv.AddItem(kGroup, key, value); });
	ReadParams();
	Restart();
}

void ClearSequence::ReadParams()
{
	ForEachParam(params_, [](const char* key, auto& value) {
		using T = std::decay_t<decltype(value)>;
		if (Global().HasItem(kGroup, key)) {
			value = Global().GetValueRef<T>(kGroup, key);
		}
	});
}

void ClearSequence::Update(float deltaTime)
{
	ReadParams();
	prevTime_ = time_;
	time_ += deltaTime;
}

void ClearSequence::Restart()
{
	time_ = 0.0f;
	prevTime_ = -1.0f;
	skipped_ = false;
}

void ClearSequence::SkipToMenu()
{
	if (IsMenuReady()) {
		return;
	}
	// 前の時刻も合わせて進めるので、途中の Crossed は1つも成立しない（SE がまとめて鳴らない）
	time_ = params_.menuStart;
	prevTime_ = time_;
	skipped_ = true;
}

float ClearSequence::Progress(float start, float duration) const
{
	if (duration <= 0.0f) {
		return time_ >= start ? 1.0f : 0.0f;
	}
	return std::clamp((time_ - start) / duration, 0.0f, 1.0f);
}

#ifdef _DEBUG
void ClearSequence::Save() const
{
	Global().SaveFile(kDirectory, kGroup);
}
#endif
