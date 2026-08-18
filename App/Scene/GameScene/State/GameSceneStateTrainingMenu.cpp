#include "GameSceneStateTrainingMenu.h"
#include "Scene/GameScene/GameScene.h"

void GameSceneStateTrainingMenu::Enter(GameScene& scene)
{
	// メニュー操作がそのままプレイヤーやカメラを動かさないように切っておく
	scene.GetInputContext()->SetCanPlayerMove(false);
	scene.GetInputContext()->SetCanLockOn(false);
	scene.GetInputContext()->SetCanCameraMove(false);

	if (TrainingMenu* menu = scene.GetTrainingMenu()) {
		menu->Open();
	}
}

void GameSceneStateTrainingMenu::Update(GameScene& scene)
{
	// 相手の出し直しはメニューからその場で行うが、時間は止めておく。
	// パネルの演出は実時間で進むので、これでも操作は効く
	scene.SetSceneTime(0.0f);

	TrainingMenu* menu = scene.GetTrainingMenu();
	if (menu && menu->GetResult() == TrainingMenu::Result::Close) {
		scene.ChangeState("Play");
	}
}

void GameSceneStateTrainingMenu::Exit(GameScene& scene)
{
	if (TrainingMenu* menu = scene.GetTrainingMenu()) {
		menu->Close();
	}

	// 開いている間に積まれた要求は捨てる。
	// キーは受け付けていないので積むのはエディタのボタンだけだが、
	// 残したままだと閉じた次のフレームに開き直してしまう
	if (TrainingController* training = scene.GetTrainingController()) {
		training->ConsumeMenuRequest();
	}
}
