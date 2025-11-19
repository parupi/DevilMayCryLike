#include "ClearScene.h"
#include <3d/Camera/Camera.h>
#include <3d/Camera/CameraManager.h>
#include <3d/SkySystem/SkySystem.h>
#include <3d/Light/LightManager.h>
#include <3d/Object/Object3dManager.h>
#include <3d/Collider/CollisionManager.h>
#include <3d/Object/Renderer/RendererManager.h>
#include <base/TextureManager.h>
#include <input/Input.h>
#include <scene/Transition/SceneTransitionController.h>
#include <GameData/GameData.h>

void ClearScene::Initialize()
{
	TextureManager::GetInstance()->LoadTexture("ClearUI.png");
	TextureManager::GetInstance()->LoadTexture("Result.png");
	TextureManager::GetInstance()->LoadTexture("Numbers.png");
	TextureManager::GetInstance()->LoadTexture("Ranks.png");
	TextureManager::GetInstance()->LoadTexture("Stage1.png");
	TextureManager::GetInstance()->LoadTexture("Score.png");

	// カメラの生成
	std::unique_ptr<Camera> clearCamera = std::make_unique<Camera>("ClearCamera");
	clearCamera->GetTranslate() = { 0.0f, 10.0f, -20.0f };
	clearCamera->GetRotate() = { -1.0f, 5.0f, -0.5f };
	CameraManager::GetInstance()->AddCamera(std::move(clearCamera));
	CameraManager::GetInstance()->SetActiveCamera("ClearCamera");

	SkySystem::GetInstance()->CreateSkyBox("qwantani_moon_noon_puresky_4k.dds");

	// ============ライト=================//
	LightManager::GetInstance()->CreateDirectionalLight("gameDir");

	clearUI_ = std::make_unique<ClearUI>();
	clearUI_->Initialize();
}

void ClearScene::Finalize()
{
	Object3dManager::GetInstance()->DeleteAllObject();
	CollisionManager::GetInstance()->DeleteAllCollider();
	RendererManager::GetInstance()->DeleteAllRenderer();
	CameraManager::GetInstance()->DeleteAllCamera();
	LightManager::GetInstance()->DeleteAllLight();
}

void ClearScene::Update()
{
	clearUI_->Update();


	if (Input::GetInstance()->IsConnected()) {
		if (Input::GetInstance()->TriggerButton(PadNumber::ButtonA)) {
			SceneTransitionController::GetInstance()->RequestSceneChange("TITLE", true);
		}
	} else {
		if (Input::GetInstance()->TriggerKey(DIK_SPACE)) {
			SceneTransitionController::GetInstance()->RequestSceneChange("TITLE", true);
		}
	}

}

void ClearScene::Draw()
{
	// 全オブジェクトの描画
	Object3dManager::GetInstance()->DrawSet();

	SpriteManager::GetInstance()->DrawSet();
	clearUI_->Draw();

}

void ClearScene::DrawRTV()
{
}

#ifdef _DEBUG
void ClearScene::DebugUpdate()
{

}
#endif // DEBUG