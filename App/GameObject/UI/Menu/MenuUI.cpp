#include "MenuUI.h"
#include "Scene/GameScene/GameScene.h"
#include <Scene/Transition/SceneTransitionController.h>

void MenuUI::Initialize(GameScene* scene) {
	scene_ = scene;

	divider_ = std::make_unique<MenuDivider>();
	divider_->Initialize();

	choices_ = std::make_unique<MenuChoices>();
	choices_->Initialize();

	controller_ = std::make_unique<MenuController>();
	controller_->Initialize();
}

void MenuUI::Enter() {
	isActive_ = true;
	isExit_ = false;
	isDecision_ = false;

	divider_->Enter();
	controller_->Enter();
	choices_->Enter();
}

void MenuUI::Exit() {
	isActive_ = false;

	// Continue選択によるExitアニメーションがすでに動いている場合はスキップ
	if (isExit_) return;

	divider_->Exit();
	choices_->Exit();
	controller_->Exit();
}

void MenuUI::Update() {
	if (isActive_) {
		// Continue選択 → Exitアニメーション開始
		if (!isExit_ && controller_->GetStates() == MenuStates::Exit) {
			isExit_ = true;
			choices_->Exit();
			divider_->Exit();
		}

		// Exitアニメーション完了後にゲーム再開
		if (isExit_ && controller_->GetStates() == MenuStates::SetUp) {
			scene_->ChangeState("Play"); // 内部でExit()が呼ばれisActive_がfalseになる
			isExit_ = false;
		}

		if (!isDecision_ && controller_->GetStates() == MenuStates::Decision) {
			isDecision_ = true;
			SceneTransitionController::GetInstance().RequestSceneChange("TITLE");
		}

		MenuStates controllerState = controller_->GetStates();
		int selectedIndex = (controllerState == MenuStates::SelectSecond ||
			controllerState == MenuStates::Confirm ||
			controllerState == MenuStates::Decision) ? 1 : 0;
		choices_->SetSelectedIndex(selectedIndex);
		choices_->SetConfirming(controllerState == MenuStates::Confirm);
	}

	choices_->Update();
	controller_->Update();
	divider_->Update();
}

void MenuUI::Draw() {
	choices_->Draw();
	divider_->Draw();
}
