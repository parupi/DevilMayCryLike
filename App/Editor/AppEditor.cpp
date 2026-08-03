#include "AppEditor.h"
#ifdef _DEBUG

#include "Windows/AppEditorWindows.h"

#include <Editor/Core/EditorHost.h>
#include <World3D/Camera/CameraManager.h>
#include <World3D/Object/Object3d.h>
#include <World3D/Object/Object3dManager.h>

#include "GameObject/Camera/GameCamera.h"
#include "GameObject/Character/Player/Player.h"

namespace {

// ゲーム側のウィンドウを含む配置。エンジンはこれらの名前を知らないので、こちらで持つ
void RegisterLayoutPresets()
{
	Editor::AddLayoutPreset({
		"バトル調整",
		{ "Game" },
		{ "Player", "Attack Editor", "Attack Derivative Editor" },
		{ "Enemy", "Stylish", "GameCamera" },
		{ "TimeManager", "DeltaTime", "Debug Log" },
		0.24f, 0.22f, 0.24f,
		});

	Editor::AddLayoutPreset({
		"カメラ調整",
		{ "Game" },
		{},
		{ "GameCamera", "Player" },
		{ "Debug Log", "TimeManager" },
		0.0f, 0.28f, 0.24f,
		});

	Editor::AddLayoutPreset({
		"VFX作業(ゲーム)",
		{ "Game" },
		{ "Particle Groups" },
		{ "Emitters", "VFX", "HitEffect", "HitPostEffect" },
		{ "Particle Node Graph", "Debug Log" },
		0.18f, 0.26f, 0.38f,
		});
}

} // namespace

Player* AppEditor::FindPlayer()
{
	Object3dManager* objects = Editor::Ctx().object3dManager;
	if (!objects) {
		return nullptr;
	}
	// FindObject は名前が要るうえ、見つからないとログを吐く。
	// 型で探せば名前が変わっても追従できる
	for (Object3d* object : objects->GetAllObject()) {
		if (auto* player = dynamic_cast<Player*>(object)) {
			return player;
		}
	}
	return nullptr;
}

GameCamera* AppEditor::FindGameCamera()
{
	CameraManager* cameras = Editor::Ctx().cameraManager;
	if (!cameras) {
		return nullptr;
	}
	// FindCamera は operator[] で引くので、無い名前を渡すと空要素が生える。
	// 登録済みの名前だけを回すこと
	for (const std::string& name : cameras->GetCameraNames()) {
		if (auto* camera = dynamic_cast<GameCamera*>(cameras->FindCamera(name))) {
			return camera;
		}
	}
	return nullptr;
}

void AppEditor::Register()
{
	RegisterLayoutPresets();

	// ゲーム固有のウィンドウ。エンジン標準のウィンドウの後ろに並ぶ
	Editor::AddWindowDrawer([] { DrawPlayerWindow(); });
	Editor::AddWindowDrawer([] { DrawAttackEditorWindows(); });
	Editor::AddWindowDrawer([] { DrawEnemyWindow(); });
	Editor::AddWindowDrawer([] { DrawCameraWorkWindow(); });
	Editor::AddWindowDrawer([] { DrawStylishWindow(); });
	Editor::AddWindowDrawer([] { DrawHitEffectWindows(); });

	// ゲーム固有のメニューを足すならここ。Help の右、再生コントロールの左に並ぶ
	//   Editor::AddMenu("Game", [] { ImGui::MenuItem("敵を沸かせる"); });
}

#endif // _DEBUG
