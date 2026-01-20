#include "PlayerStateDeath.h"
#include <base/utility/DeltaTime.h>
#include <algorithm>
#include "GameObject/Character/Player/Player.h"
#include <scene/Transition/SceneTransitionController.h>
#include <scene/Transition/TransitionManager.h>
#include <scene/Transition/VignetteExpandTransition.h>
#include <math/Easing.h>
#include <GameObject/Camera/DeathCamera.h>

PlayerStateDeath::PlayerStateDeath()
{
	// ステート生成時に一度トランジションを生成
	if (TransitionManager::GetInstance()->AddTransition(std::make_unique<VignetteExpandTransition>("Death"))) {
		// 追加に成功したら初期化
		static_cast<VignetteExpandTransition*>(TransitionManager::GetInstance()->GetCurrentTransition())->Initialize();
		// 生成した段階ではフェードをセットしておく
		TransitionManager::GetInstance()->SetTransition("Fade");
	}
}

void PlayerStateDeath::Enter(Player& player)
{
	auto cameraManager = CameraManager::GetInstance();

	// 現在のゲームカメラを取得
	Camera* current = cameraManager->GetActiveCamera();

	// デスカメラ生成
	std::unique_ptr<DeathCamera> deathCam = std::make_unique<DeathCamera>("DeathCamera", current, &player);

	// 差し替え
	cameraManager->AddCamera(std::move(deathCam));


	// プレイヤーの初期姿勢を記録
	startRotate_ = player.GetWorldTransform()->GetRotation();
	startScale_ = player.GetWorldTransform()->GetScale();

	// 死亡時の最終姿勢
	endRotate_ = startRotate_ + EulerDegree({90.0f, 90.0f, 0.0f});

	// 死亡時の最終スケール（完全に消える直前まで縮小）
	endScale_ = Vector3{ 0.0f, 0.0f, 0.0f };

	// タイマー初期化
	currentTime_ = 0.0f;
}

void PlayerStateDeath::Update(Player& player)
{
	if (currentTime_ <= 0.0f) {
		CameraManager::GetInstance()->SetActiveCamera("DeathCamera");
	}

	// 時間経過
	currentTime_ += DeltaTime::GetDeltaTime();

	// 0.0～1.0に正規化
	float t = std::clamp(currentTime_ / maxTime_, 0.0f, 1.0f);

	// イージングを使いたい場合（自然な減速回転など）
	float easedT = easeInCubic(t);

	// 回転補間
	Quaternion currentRotate = Slerp(startRotate_, endRotate_, t);

	// 拡縮補間
	Vector3 currentScale = Lerp(startScale_, endScale_, easedT);

	// プレイヤーに適用
	player.GetWorldTransform()->GetRotation() = currentRotate;
	player.GetWorldTransform()->GetScale() = currentScale;

	// 完了チェック
	if (t >= 1.0f) {
		player.ChangeState("Idle");
	}
}

void PlayerStateDeath::Exit(Player& player)
{
	// シーン遷移を設定
	TransitionManager::GetInstance()->SetTransition("Death");

	// シーンを変える
	SceneTransitionController::GetInstance()->RequestSceneChange("GAMEPLAY");
}
