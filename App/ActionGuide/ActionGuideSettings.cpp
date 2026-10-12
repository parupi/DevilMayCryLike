#include "ActionGuideSettings.h"

#include <Debugger/GlobalVariables.h>

#include <algorithm>

namespace {
	GlobalVariables& Global() { return GlobalVariables::GetInstance(); }
	constexpr const char* kGroup = ActionGuideSettings::kGroupName;
}

const char* ActionGuideSettings::PriorityKey(ActionGuideCategory category)
{
	switch (category) {
	case ActionGuideCategory::ChargeRelease: return "PriorityChargeRelease";
	case ActionGuideCategory::Counter:       return "PriorityCounter";
	case ActionGuideCategory::ComboBranch:   return "PriorityComboBranch";
	case ActionGuideCategory::BranchLater:   return "PriorityBranchLater";
	case ActionGuideCategory::RootInterrupt: return "PriorityRootInterrupt";
	case ActionGuideCategory::RootAttack:    return "PriorityRootAttack";
	case ActionGuideCategory::Dodge:         return "PriorityDodge";
	case ActionGuideCategory::Jump:          return "PriorityJump";
	case ActionGuideCategory::LockOn:        return "PriorityLockOn";
	default:                                 return "PriorityUnknown";
	}
}

const char* ActionGuideSettings::CategoryLabel(ActionGuideCategory category)
{
	switch (category) {
	case ActionGuideCategory::ChargeRelease: return "溜め中に離す";
	case ActionGuideCategory::Counter:       return "カウンター";
	case ActionGuideCategory::ComboBranch:   return "コンボの派生";
	case ActionGuideCategory::BranchLater:   return "少し待つと出る派生";
	case ActionGuideCategory::RootInterrupt: return "別ボタンでの割り込み";
	case ActionGuideCategory::RootAttack:    return "通常時の攻撃";
	case ActionGuideCategory::Dodge:         return "回避";
	case ActionGuideCategory::Jump:          return "ジャンプ";
	case ActionGuideCategory::LockOn:        return "ロックオン";
	default:                                 return "?";
	}
}

int32_t ActionGuideSettings::DefaultPriority(ActionGuideCategory category)
{
	// 仕様書 §6-5「コンボ派生 → 回避 → 基本操作」の順。
	// 溜め中に離す・カウンターは出せる時間が短いので、さらに上に置く
	switch (category) {
	case ActionGuideCategory::ChargeRelease: return 100;
	case ActionGuideCategory::Counter:       return 95;
	case ActionGuideCategory::ComboBranch:   return 80;
	case ActionGuideCategory::BranchLater:   return 75;
	case ActionGuideCategory::RootInterrupt: return 70;
	case ActionGuideCategory::RootAttack:    return 60;
	case ActionGuideCategory::Dodge:         return 40;
	case ActionGuideCategory::Jump:          return 20;
	case ActionGuideCategory::LockOn:        return 10;
	default:                                 return 0;
	}
}

void ActionGuideSettings::RegisterAndLoad()
{
	GlobalVariables& gv = Global();
	gv.CreateGroup(kGroup);
	// 保存済みの設定があれば読み込む（無ければ AddItem の既定値がそのまま残る）
	gv.LoadFile(kDirectoryName, kGroup);

	gv.AddItem(kGroup, "MaxEntries", maxEntries);
	gv.AddItem(kGroup, "ShowPreview", showPreview);
	gv.AddItem(kGroup, "ShowLockOn", showLockOn);
	gv.AddItem(kGroup, "ShowJump", showJump);
	gv.AddItem(kGroup, "ShowDodge", showDodge);
	for (int32_t i = 0; i < kActionGuideCategoryCount; ++i) {
		const auto category = static_cast<ActionGuideCategory>(i);
		gv.AddItem(kGroup, PriorityKey(category), DefaultPriority(category));
	}

	gv.AddItem(kGroup, "NameJump", nameJump);
	gv.AddItem(kGroup, "NameDodge", nameDodge);
	gv.AddItem(kGroup, "NameLockOn", nameLockOn);
	gv.AddItem(kGroup, "TextHold", textHold);
	gv.AddItem(kGroup, "TextRelease", textRelease);
	gv.AddItem(kGroup, "TextWait", textWait);

	gv.AddItem(kGroup, "IconX", iconX);
	gv.AddItem(kGroup, "BottomY", bottomY);
	gv.AddItem(kGroup, "TutorialRoomRaise", tutorialRoomRaise);
	gv.AddItem(kGroup, "RowSpacing", rowSpacing);
	gv.AddItem(kGroup, "IconSize", iconSize);
	gv.AddItem(kGroup, "FontSize", fontSize);
	gv.AddItem(kGroup, "GestureFontSize", gestureFontSize);
	gv.AddItem(kGroup, "TextGap", textGap);
	gv.AddItem(kGroup, "BackdropAlpha", backdropAlpha);
	gv.AddItem(kGroup, "BackdropWidth", backdropWidth);

	gv.AddItem(kGroup, "NormalAlpha", normalAlpha);
	gv.AddItem(kGroup, "PreviewAlpha", previewAlpha);
	gv.AddItem(kGroup, "FadeTime", fadeTime);
	gv.AddItem(kGroup, "FadeStartAlpha", fadeStartAlpha);
	gv.AddItem(kGroup, "SlideOffset", slideOffset);
	gv.AddItem(kGroup, "QueuedMarkSize", queuedMarkSize);

	Apply();
}

void ActionGuideSettings::Apply()
{
	GlobalVariables& gv = Global();
	if (!gv.HasItem(kGroup, "MaxEntries")) return;

	maxEntries = std::clamp(gv.GetValueRef<int32_t>(kGroup, "MaxEntries"), 1, kMaxRows);
	showPreview = gv.GetValueRef<bool>(kGroup, "ShowPreview");
	showLockOn = gv.GetValueRef<bool>(kGroup, "ShowLockOn");
	showJump = gv.GetValueRef<bool>(kGroup, "ShowJump");
	showDodge = gv.GetValueRef<bool>(kGroup, "ShowDodge");
	for (int32_t i = 0; i < kActionGuideCategoryCount; ++i) {
		priorities[i] = gv.GetValueRef<int32_t>(kGroup, PriorityKey(static_cast<ActionGuideCategory>(i)));
	}

	nameJump = gv.GetValueRef<std::string>(kGroup, "NameJump");
	nameDodge = gv.GetValueRef<std::string>(kGroup, "NameDodge");
	nameLockOn = gv.GetValueRef<std::string>(kGroup, "NameLockOn");
	textHold = gv.GetValueRef<std::string>(kGroup, "TextHold");
	textRelease = gv.GetValueRef<std::string>(kGroup, "TextRelease");
	textWait = gv.GetValueRef<std::string>(kGroup, "TextWait");

	iconX = gv.GetValueRef<float>(kGroup, "IconX");
	bottomY = gv.GetValueRef<float>(kGroup, "BottomY");
	tutorialRoomRaise = gv.GetValueRef<float>(kGroup, "TutorialRoomRaise");
	rowSpacing = gv.GetValueRef<float>(kGroup, "RowSpacing");
	iconSize = gv.GetValueRef<float>(kGroup, "IconSize");
	fontSize = gv.GetValueRef<float>(kGroup, "FontSize");
	gestureFontSize = gv.GetValueRef<float>(kGroup, "GestureFontSize");
	textGap = gv.GetValueRef<float>(kGroup, "TextGap");
	backdropAlpha = gv.GetValueRef<float>(kGroup, "BackdropAlpha");
	backdropWidth = gv.GetValueRef<float>(kGroup, "BackdropWidth");

	normalAlpha = gv.GetValueRef<float>(kGroup, "NormalAlpha");
	previewAlpha = gv.GetValueRef<float>(kGroup, "PreviewAlpha");
	fadeTime = gv.GetValueRef<float>(kGroup, "FadeTime");
	fadeStartAlpha = gv.GetValueRef<float>(kGroup, "FadeStartAlpha");
	slideOffset = gv.GetValueRef<float>(kGroup, "SlideOffset");
	queuedMarkSize = gv.GetValueRef<float>(kGroup, "QueuedMarkSize");
}

void ActionGuideSettings::Save()
{
	Global().SaveFile(kDirectoryName, kGroup);
}
