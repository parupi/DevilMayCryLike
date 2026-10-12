#pragma once
#include "ActionGuideTypes.h"
#include "ActionGuideContext.h"
#include "ActionAvailabilityProvider.h"

#include <vector>

class ActionGuideCatalog;
struct ActionGuideSettings;

/// <summary>ガイドをいつ出すか（OPTION の「ACTION GUIDE」）</summary>
enum class ActionGuideMode : int32_t {
	Off = 0,        ///< 出さない
	BattleOnly = 1, ///< 戦闘中だけ
	Always = 2,     ///< 常に
};

/// <summary>
/// 候補の取得・評価・絞り込み・並べ替え・件数の制限まで（仕様書 §5.4 / §6）。
/// 描画の API には依存しない。結果は ViewModel として UI に渡す
/// </summary>
class ActionGuideSystem
{
public:
	void Update(const ActionGuideContext& context, const IActionAvailabilityProvider& provider,
		const ActionGuideCatalog& catalog, const ActionGuideSettings& settings, ActionGuideMode mode);

	const ActionGuideViewModel& GetViewModel() const { return viewModel_; }

	/// <summary>前のフレームから中身が変わったか（UI の切り替えアニメーションの合図）</summary>
	bool IsChanged() const { return changed_; }

	/// <summary>ガイドを隠している理由（エディタのデバッグ表示用）。出しているときは空</summary>
	const char* GetHiddenReason() const { return hiddenReason_; }
	/// <summary>最後に使った状態（エディタのデバッグ表示用）</summary>
	const ActionGuideContext& GetLastContext() const { return lastContext_; }

	/// <summary>
	/// 今動いているガイド。エディタの Action Guide ウィンドウが引く（エディタは GameScene を知らない作りのため）。
	/// 所有は GameScene で、Initialize / Finalize で付け外しする
	/// </summary>
	static const ActionGuideSystem* GetCurrent() { return current_; }
	static void SetCurrent(const ActionGuideSystem* system) { current_ = system; }

private:
	ActionGuideViewModel viewModel_;
	bool changed_ = false;
	const char* hiddenReason_ = "";
	ActionGuideContext lastContext_;
	static inline const ActionGuideSystem* current_ = nullptr;
	std::vector<ActionGuideCandidate> candidates_;
};
