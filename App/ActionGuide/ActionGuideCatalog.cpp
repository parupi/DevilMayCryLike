#include "ActionGuideCatalog.h"
#include "ActionGuideSettings.h"

#include <Debugger/GlobalVariables.h>

ActionGuideID ActionGuideCatalog::GetAttackId(const std::string& attackName)
{
	const auto it = attackIds_.find(attackName);
	if (it != attackIds_.end()) return it->second;

	const ActionGuideID id = nextAttackId_++;
	attackIds_[attackName] = id;
	attackNames_[id] = attackName;
	return id;
}

const std::string& ActionGuideCatalog::GetAttackName(ActionGuideID id) const
{
	static const std::string kEmpty;
	const auto it = attackNames_.find(id);
	return (it != attackNames_.end()) ? it->second : kEmpty;
}

std::string ActionGuideCatalog::GetDisplayName(ActionGuideID id, const ActionGuideSettings& settings) const
{
	switch (id) {
	case kJump:       return settings.nameJump;
	case kDodge:      return settings.nameDodge;
	case kLockOn:     return settings.nameLockOn;
	default:          break;
	}

	const std::string& attackName = GetAttackName(id);
	if (attackName.empty()) return std::string();

	// 攻撃エディタで編集した値をそのまま読む（毎フレーム読むので、書き換えるとすぐ反映される）
	GlobalVariables& gv = GlobalVariables::GetInstance();
	if (gv.HasItem(attackName, "GuideName")) {
		const std::string& guideName = gv.GetValueRef<std::string>(attackName, "GuideName");
		if (!guideName.empty()) return guideName;
	}
	return attackName;
}

bool ActionGuideCatalog::IsVisibleInGuide(ActionGuideID id) const
{
	const std::string& attackName = GetAttackName(id);
	if (attackName.empty()) return true;

	GlobalVariables& gv = GlobalVariables::GetInstance();
	if (!gv.HasItem(attackName, "GuideVisible")) return true;
	return gv.GetValueRef<bool>(attackName, "GuideVisible");
}
