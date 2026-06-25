#include "ClearScene.h"
#include <World3D/Camera/BaseCamera.h>
#include <World3D/Camera/CameraManager.h>
#include <Graphics/Rendering/Sky/SkySystem.h>
#include <World3D/Light/LightManager.h>
#include <World3D/Object/Object3dManager.h>
#include <World3D/Collider/CollisionManager.h>
#include <World3D/Object/Renderer/RendererManager.h>
#include "Graphics/Resource/TextureManager.h"
#include <Input/Input.h>
#include <Scene/Transition/SceneTransitionController.h>
#include <GameData/GameData.h>
#include "Graphics/Rendering/Sprite/SpriteManager.h"

void ClearScene::Initialize()
{
	TextureManager::GetInstance().LoadTexture("ClearUI.png");
	TextureManager::GetInstance().LoadTexture("Result.png");
	TextureManager::GetInstance().LoadTexture("Numbers.png");
	TextureManager::GetInstance().LoadTexture("Ranks.png");
	TextureManager::GetInstance().LoadTexture("Stage1.png");
	TextureManager::GetInstance().LoadTexture("Score.png");

	// カメラの生成
	std::unique_ptr<BaseCamera> clearCamera = std::make_unique<BaseCamera>("ClearCamera");
	clearCamera->GetTranslate() = { 0.0f, 10.0f, -20.0f };
	clearCamera->GetRotate() = { -1.0f, 5.0f, -0.5f };
	CameraManager::GetInstance().AddCamera(std::move(clearCamera));
	CameraManager::GetInstance().SetActiveCamera("ClearCamera");

	SkySystem::GetInstance().CreateSkyBox("qwantani_moon_noon_puresky_4k.dds");

	// ============ライト=================//
	//LightManager::GetInstance().CreateDirectionalLight("gameDir");

	clearUI_ = std::make_unique<ClearUI>();
	clearUI_->Initialize();
}

void ClearScene::Finalize()
{
	SpriteManager::GetInstance().DeleteNonPersistentSprite();
	Object3dManager::GetInstance().DeleteAllObject();
	CollisionManager::GetInstance().DeleteAllCollider();
	RendererManager::GetInstance().DeleteAllRenderer();
	CameraManager::GetInstance().DeleteAllCamera();
	LightManager::GetInstance().DeleteAllLight();
}

void ClearScene::Update()
{
	clearUI_->Update();


	if (Input::GetInstance().IsConnected()) {
		if (Input::GetInstance().TriggerButton(PadNumber::ButtonA)) {
			SceneTransitionController::GetInstance().RequestSceneChange("TITLE", true);
		}
	} else {
		if (Input::GetInstance().TriggerKey(DIK_SPACE)) {
			SceneTransitionController::GetInstance().RequestSceneChange("TITLE", true);
		}
	}

}

void ClearScene::Draw()
{
	// 全オブジェクトの描画
	//Object3dManager::GetInstance().DrawSet();

	SpriteManager::GetInstance().DrawSet();
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