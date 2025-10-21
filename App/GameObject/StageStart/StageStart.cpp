#include "StageStart.h"
#include <memory>
#include <3d/Camera/CameraManager.h>
#include <3d/Camera/Camera.h>
#include <scene/Transition/TransitionManager.h>

void StageStart::Initialize()
{
	std::unique_ptr<Camera> cam = std::make_unique<Camera>("StartCamera1");
	cam->GetTranslate() = { -80.0f, 60.0f, 30.0f };
	cam->GetRotate() = { 0.58f, 2.0f, 0.0f };

	CameraManager::GetInstance()->AddCamera(std::move(cam));
	CameraManager::GetInstance()->SetActiveCamera("StartCamera1");

	cam = std::make_unique<Camera>("StartCamera2");
	cam->GetTranslate() = { -80.0f, 60.0f, 35.0f };
	cam->GetRotate() = { 0.58f, 1.1f, 0.0f };
	CameraManager::GetInstance()->AddCamera(std::move(cam));

	cam = std::make_unique<Camera>("StartCamera3");
	cam->GetTranslate() = { -50.0f, 60.0f, 80.0f };
	cam->GetRotate() = { 0.54f, 2.6f, 0.0f };
	CameraManager::GetInstance()->AddCamera(std::move(cam));

	cam = std::make_unique<Camera>("StartCamera4");
	cam->GetTranslate() = { 30.0f, 65.0f, 100.0f };
	cam->GetRotate() = { 0.54f, 3.5f, 0.0f };
	CameraManager::GetInstance()->AddCamera(std::move(cam));

	cam = std::make_unique<Camera>("StartCamera5");
	cam->GetTranslate() = { 60.0f, 45.0f, 40.0f };
	cam->GetRotate() = { 0.54f, 4.2f, 0.0f };
	CameraManager::GetInstance()->AddCamera(std::move(cam));

	cam = std::make_unique<Camera>("StartCamera6");
	CameraManager::GetInstance()->AddCamera(std::move(cam));
}

void StageStart::Update()
{
	if (TransitionManager::GetInstance()->IsFinished() && !CameraManager::GetInstance()->IsTransition()) {
		if (CameraManager::GetInstance()->GetActiveCamera()->name_ == "StartCamera1") {
			CameraManager::GetInstance()->SetActiveCamera("StartCamera2", 2.0f);
		} else if (CameraManager::GetInstance()->GetActiveCamera()->name_ == "StartCamera2") {
			CameraManager::GetInstance()->SetActiveCamera("StartCamera3", 1.0f);
		} else if (CameraManager::GetInstance()->GetActiveCamera()->name_ == "StartCamera3") {
			CameraManager::GetInstance()->SetActiveCamera("StartCamera4", 1.5f);
		} else if (CameraManager::GetInstance()->GetActiveCamera()->name_ == "StartCamera4") {
			CameraManager::GetInstance()->SetActiveCamera("StartCamera5", 1.5f);
		} else if (CameraManager::GetInstance()->GetActiveCamera()->name_ == "StartCamera5") {
			CameraManager::GetInstance()->SetActiveCamera("StartCamera6");
		} else if (CameraManager::GetInstance()->GetActiveCamera()->name_ == "StartCamera6") {
			CameraManager::GetInstance()->SetActiveCamera("GameCamera", 2.0f);
		}
	}
}
