#include "GameSettings.h"
#include "Debugger/GlobalVariables.h"
#include "Input/GamepadRumble.h"

#include <algorithm>

namespace {
	// Resource/GlobalVariables/Settings/GameSettings.json
	const char* kDirectory = "Settings";
	const char* kGroup = "GameSettings";
	const char* kKeySensitivity = "CameraSensitivity";
	const char* kKeyInvertY = "InvertCameraY";
	const char* kKeyTutorial = "TutorialEnabled";
	const char* kKeyActionGuide = "ActionGuideMode";
	const char* kKeyVibration = "Vibration";
}

GameSettings& GameSettings::GetInstance() {
	static GameSettings instance;
	// シーンより先に読まれることがあるので、初期化のタイミングを呼び出し側に委ねない
	if (!instance.isInitialized_) {
		instance.Initialize();
	}
	return instance;
}

void GameSettings::Initialize() {
	if (isInitialized_) { return; }
	// 先に立てておかないと、この下の GetInstance 経由で再入する
	isInitialized_ = true;

	GlobalVariables* global = &GlobalVariables::GetInstance();
	global->LoadFile(kDirectory, kGroup);
	global->AddItem(kGroup, kKeySensitivity, cameraSensitivity_);
	global->AddItem(kGroup, kKeyInvertY, invertCameraY_);
	global->AddItem(kGroup, kKeyTutorial, tutorialEnabled_);
	global->AddItem(kGroup, kKeyActionGuide, actionGuideMode_);
	global->AddItem(kGroup, kKeyVibration, vibration_);
	PullValues();
}

void GameSettings::Save() {
	GlobalVariables::GetInstance().SaveFile(kDirectory, kGroup);
}

void GameSettings::SetCameraSensitivity(float scale) {
	cameraSensitivity_ = std::clamp(scale, kMinSensitivity, kMaxSensitivity);
	GlobalVariables::GetInstance().SetValue(kGroup, kKeySensitivity, cameraSensitivity_);
}

void GameSettings::SetInvertCameraY(bool invert) {
	invertCameraY_ = invert;
	GlobalVariables::GetInstance().SetValue(kGroup, kKeyInvertY, invertCameraY_);
}

void GameSettings::SetTutorialEnabled(bool enabled) {
	tutorialEnabled_ = enabled;
	GlobalVariables::GetInstance().SetValue(kGroup, kKeyTutorial, tutorialEnabled_);
}

void GameSettings::SetActionGuideMode(int32_t mode) {
	actionGuideMode_ = std::clamp(mode, 0, kActionGuideModeCount - 1);
	GlobalVariables::GetInstance().SetValue(kGroup, kKeyActionGuide, actionGuideMode_);
}

void GameSettings::SetVibration(float scale) {
	vibration_ = std::clamp(scale, 0.0f, 1.0f);
	GlobalVariables::GetInstance().SetValue(kGroup, kKeyVibration, vibration_);
	GamepadRumble::GetInstance().SetMasterScale(vibration_);
}

void GameSettings::PullValues() {
	GlobalVariables* global = &GlobalVariables::GetInstance();
	if (!global->HasItem(kGroup, kKeySensitivity)) { return; }

	cameraSensitivity_ = std::clamp(global->GetValueRef<float>(kGroup, kKeySensitivity), kMinSensitivity, kMaxSensitivity);
	invertCameraY_ = global->GetValueRef<bool>(kGroup, kKeyInvertY);
	tutorialEnabled_ = global->GetValueRef<bool>(kGroup, kKeyTutorial);
	if (global->HasItem(kGroup, kKeyVibration)) {
		vibration_ = std::clamp(global->GetValueRef<float>(kGroup, kKeyVibration), 0.0f, 1.0f);
	}
	// 振動の部品はエンジン側なので、読んだ値をここで渡しておく
	GamepadRumble::GetInstance().SetMasterScale(vibration_);
	if (global->HasItem(kGroup, kKeyActionGuide)) {
		actionGuideMode_ = std::clamp(global->GetValueRef<int32_t>(kGroup, kKeyActionGuide), 0, kActionGuideModeCount - 1);
	}
}
