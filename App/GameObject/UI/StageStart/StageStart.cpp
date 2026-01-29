#include "StageStart.h"
#include <memory>
#include <3d/Camera/CameraManager.h>
#include "3d/Camera/BaseCamera.h"
#include <scene/Transition/TransitionManager.h>
#include <input/Input.h>

void StageStart::Initialize()
{
	std::unique_ptr<BaseCamera> cam = std::make_unique<BaseCamera>("StartCamera1");
	cam->GetTranslate() = { -80.0f, 60.0f, 30.0f };
	cam->GetRotate() = { 0.58f, 2.0f, 0.0f };

	CameraManager::GetInstance()->AddCamera(std::move(cam));
	CameraManager::GetInstance()->SetActiveCamera("StartCamera1");

	cam = std::make_unique<BaseCamera>("StartCamera2");
	cam->GetTranslate() = { -80.0f, 60.0f, 35.0f };
	cam->GetRotate() = { 0.58f, 1.1f, 0.0f };
	CameraManager::GetInstance()->AddCamera(std::move(cam));

	cam = std::make_unique<BaseCamera>("StartCamera3");
	cam->GetTranslate() = { -50.0f, 60.0f, 80.0f };
	cam->GetRotate() = { 0.54f, 2.6f, 0.0f };
	CameraManager::GetInstance()->AddCamera(std::move(cam));

	cam = std::make_unique<BaseCamera>("StartCamera4");
	cam->GetTranslate() = { 30.0f, 65.0f, 100.0f };
	cam->GetRotate() = { 0.54f, 3.5f, 0.0f };
	CameraManager::GetInstance()->AddCamera(std::move(cam));

	cam = std::make_unique<BaseCamera>("StartCamera5");
	cam->GetTranslate() = { 60.0f, 45.0f, 40.0f };
	cam->GetRotate() = { 0.54f, 4.2f, 0.0f };
	CameraManager::GetInstance()->AddCamera(std::move(cam));

	cam = std::make_unique<BaseCamera>("StartCamera6");
	CameraManager::GetInstance()->AddCamera(std::move(cam));
}

void StageStart::Update()
{
	// スタート処理が終わってたら何もせずにreturn
	if (isComplete_) return;

	// スペースを押したらスキップ
	if (Input::GetInstance()->IsConnected()) {
		if (Input::GetInstance()->PushButton(PadNumber::ButtonA)) {
			CameraManager::GetInstance()->SetActiveCamera("GameCamera", 0.3f);
			isComplete_ = true;
			return;
		}
	}else if (Input::GetInstance()->TriggerKey(DIK_SPACE) || Input::GetInstance()->TriggerKey(DIK_R)) {
		CameraManager::GetInstance()->SetActiveCamera("GameCamera", 0.3f);
		isComplete_ = true;
		return;
	}

	// 「シーン遷移が終わっていないか」「カメラの移動が行われているか」で早期リターン
	if (!TransitionManager::GetInstance()->IsFinished() || CameraManager::GetInstance()->IsTransition()) return;

	if (CameraManager::GetInstance()->GetActiveCamera()->name_ == "StartCamera1") {
		CameraManager::GetInstance()->SetActiveCamera("StartCamera2", 2.0f);
		return;
	} else if (CameraManager::GetInstance()->GetActiveCamera()->name_ == "StartCamera2") {
		CameraManager::GetInstance()->SetActiveCamera("StartCamera3", 1.0f);
		return;
	} else if (CameraManager::GetInstance()->GetActiveCamera()->name_ == "StartCamera3") {
		CameraManager::GetInstance()->SetActiveCamera("StartCamera4", 1.5f);
		return;
	} else if (CameraManager::GetInstance()->GetActiveCamera()->name_ == "StartCamera4") {
		CameraManager::GetInstance()->SetActiveCamera("StartCamera5", 1.5f);
		return;
	} else if (CameraManager::GetInstance()->GetActiveCamera()->name_ == "StartCamera5") {
		CameraManager::GetInstance()->SetActiveCamera("StartCamera6");
		return;
	} else if (CameraManager::GetInstance()->GetActiveCamera()->name_ == "StartCamera6") {
		CameraManager::GetInstance()->SetActiveCamera("GameCamera", 2.0f);
		return;
	}

	// ここまで来たら完了
	isComplete_ = true;
}