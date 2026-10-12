#pragma once
#include "ActionGuideTypes.h"

#include <string>
#include <unordered_map>

struct ActionGuideSettings;

/// <summary>
/// ガイドの表示定義（仕様書 §5.2）。
///
/// 攻撃以外（ジャンプ・回避・ロックオン）は固定の ID を持ち、名前は ActionGuideSettings。
/// 攻撃は攻撃データの名前から ID を割り当て、表示名は攻撃データの GuideName（攻撃エディタで編集）を読む。
/// ActionGuideID と攻撃名は別物として扱い、混同しない
/// </summary>
class ActionGuideCatalog
{
public:
	// 攻撃以外の行動の ID
	static constexpr ActionGuideID kJump = 1;
	static constexpr ActionGuideID kDodge = 2;
	static constexpr ActionGuideID kLockOn = 3;
	/// <summary>攻撃の ID はここから順に振る</summary>
	static constexpr ActionGuideID kAttackIdBase = 1000;

	/// <summary>攻撃名に対応する ID。初めて見た攻撃には新しい ID を振る</summary>
	ActionGuideID GetAttackId(const std::string& attackName);

	/// <summary>表示名。攻撃は GuideName（空なら攻撃名）、それ以外は設定の名前</summary>
	std::string GetDisplayName(ActionGuideID id, const ActionGuideSettings& settings) const;

	/// <summary>攻撃をガイドに出してよいか（攻撃データの GuideVisible）。攻撃以外は常に true</summary>
	bool IsVisibleInGuide(ActionGuideID id) const;

	/// <summary>攻撃の ID から攻撃名を引く。攻撃でなければ空</summary>
	const std::string& GetAttackName(ActionGuideID id) const;

private:
	std::unordered_map<std::string, ActionGuideID> attackIds_;
	std::unordered_map<ActionGuideID, std::string> attackNames_;
	ActionGuideID nextAttackId_ = kAttackIdBase;
};
