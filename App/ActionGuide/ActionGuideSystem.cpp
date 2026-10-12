#include "ActionGuideSystem.h"
#include "ActionAvailabilityProvider.h"
#include "ActionGuideCatalog.h"
#include "ActionGuideSettings.h"

#include <algorithm>

namespace {
	// 並べ替えの段：今押せるもの（Executable / Buffered）を上、Preview を下（仕様書 §8）
	int32_t AvailabilityRank(ActionAvailability availability) {
		return (availability == ActionAvailability::Preview) ? 1 : 0;
	}

	// 隠す状態（仕様書 §3 の Stun / Death / Menu / Cutscene）。理由はエディタに出す
	const char* ResolveHiddenReason(const ActionGuideContext& context, ActionGuideMode mode) {
		if (mode == ActionGuideMode::Off) return "OPTION で OFF";
		if (context.isDead) return "死亡演出中";
		if (context.isStunned) return "ノックバック中";
		if (context.isInputLocked) return "入力を受け付けていない（開始演出・ポーズ・クリア）";
		if (context.isInCutscene) return "演出カメラ中";
		if (context.state.empty()) return "プレイヤーのステートが無い";
		if (mode == ActionGuideMode::BattleOnly && !context.isInBattle) return "戦闘中のみ表示";
		return "";
	}
}

void ActionGuideSystem::Update(const ActionGuideContext& context, const IActionAvailabilityProvider& provider,
	const ActionGuideCatalog& catalog, const ActionGuideSettings& settings, ActionGuideMode mode)
{
	ActionGuideViewModel next;
	lastContext_ = context;

	hiddenReason_ = ResolveHiddenReason(context, mode);
	if (hiddenReason_[0] == '\0') {
		next.visible = true;

		// 1〜3. 候補を集めて評価する
		candidates_.clear();
		provider.CollectCandidates(context, candidates_);

		for (const ActionGuideCandidate& candidate : candidates_) {
			bool queued = false;
			const ActionAvailability availability = provider.Evaluate(candidate, context, queued);

			// 4. Unavailable は出さない。Preview は設定で切れる
			if (availability == ActionAvailability::Unavailable) continue;
			if (availability == ActionAvailability::Preview && !settings.showPreview) continue;

			ActionGuideEntry entry;
			entry.id = candidate.id;
			entry.binding = candidate.binding;
			entry.displayName = catalog.GetDisplayName(candidate.id, settings);
			entry.availability = availability;
			entry.priority = settings.priorities[static_cast<int32_t>(candidate.category)];
			entry.queued = queued;
			next.entries.push_back(std::move(entry));
		}

		// 5. 優先度順（今押せるものを先に）
		std::stable_sort(next.entries.begin(), next.entries.end(), [](const ActionGuideEntry& a, const ActionGuideEntry& b) {
			const int32_t rankA = AvailabilityRank(a.availability);
			const int32_t rankB = AvailabilityRank(b.availability);
			if (rankA != rankB) return rankA < rankB;
			if (a.priority != b.priority) return a.priority > b.priority;
			return a.id < b.id;
		});

		// 重複の解決：同じ入力の組み合わせが2つ以上あれば、上に来た方（実際に優先される方）だけを残す。
		// 同じ技が別の入力で2回出るのも避ける
		std::vector<ActionGuideEntry> unique;
		for (ActionGuideEntry& entry : next.entries) {
			const bool duplicated = std::any_of(unique.begin(), unique.end(), [&](const ActionGuideEntry& kept) {
				return kept.binding == entry.binding || kept.id == entry.id;
			});
			if (!duplicated) unique.push_back(std::move(entry));
		}
		next.entries = std::move(unique);

		// 6. 件数を絞る
		if (static_cast<int32_t>(next.entries.size()) > settings.maxEntries) {
			next.entries.resize(settings.maxEntries);
		}
	}

	// 7. 前フレームから変わったときだけ UI の切り替えを始める
	changed_ = !(next == viewModel_);
	viewModel_ = std::move(next);
}
