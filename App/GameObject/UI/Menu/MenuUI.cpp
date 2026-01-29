#include "MenuUI.h"
#include "scene/GameScene/GameScene.h"
#include <scene/Transition/SceneTransitionController.h>

void MenuUI::Initialize(GameScene* scene)
{
	scene_ = scene;

	divider_ = std::make_unique<MenuDivider>();
	divider_->Initialize();

	choices_ = std::make_unique<MenuChoices>();
	choices_->Initialize();

	controller_ = std::make_unique<MenuController>();
	controller_->Initialize();
}

void MenuUI::Enter()
{
	divider_->Enter();

	controller_->Enter();
	choices_->Enter();
}

void MenuUI::Exit()
{
	divider_->Exit();
	choices_->Exit();
	controller_->Exit();
}

void MenuUI::Update()
{
	if (controller_->GetStates() == MenuStates::Exit) {
		scene_->ChangeState("Play");
	}

	if (controller_->GetStates() == MenuStates::Decision) {
		SceneTransitionController::GetInstance()->RequestSceneChange("TITLE");
	}

	choices_->Update();
	controller_->Update();
	divider_->Update();
}

void MenuUI::Draw()
{
	choices_->Draw();
	controller_->Draw();
	divider_->Draw();
}
