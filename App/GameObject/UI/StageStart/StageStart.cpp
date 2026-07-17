#include "StageStart.h"
#include <memory>
#include <World3D/Camera/CameraManager.h>
#include "World3D/Camera/BaseCamera.h"
#include <World3D/Object/Object3d.h>
#include <Scene/Transition/TransitionManager.h>
#include <Input/Input.h>

namespace {
// プレイヤーのアップから引いていく演出の調整パラメータ
constexpr float kCloseUpDistance = 3.5f; // アップ時のプレイヤーからの水平距離[m]
constexpr float kCloseUpHeight = 1.0f;   // アップ時のカメラ高さオフセット[m]
constexpr float kLookAtHeight = 0.8f;    // 注視点の高さ（プレイヤーの上半身）[m]
constexpr float kPullBackTime = 2.5f;    // 引き（ドリーアウト）の所要時間[s]
} // namespace

void StageStart::Initialize()
{
	// プレイヤーの背後アップから始まるカメラを作る。
	// 配置データの回転を考慮し、プレイヤーの向き基準で真後ろに置く
	Vector3 playerPos{};
	Vector3 playerForward{ 0.0f, 0.0f, 1.0f };
	if (auto* player = Object3dManager::GetInstance().FindObject("Player")) {
		playerPos = player->GetWorldTransform()->GetTranslation();
		// 初回Update前でワールド行列が未計算のため、回転クォータニオンから前方向を求める
		playerForward = RotateVector({ 0.0f, 0.0f, 1.0f }, player->GetWorldTransform()->GetRotation());
		playerForward.y = 0.0f; // 水平方向のみ
		if (Length(playerForward) > 0.001f) {
			playerForward = Normalize(playerForward);
		} else {
			playerForward = { 0.0f, 0.0f, 1.0f };
		}
	}

	auto cam = std::make_unique<BaseCamera>("StartCamera");
	cam->GetTranslate() = playerPos - playerForward * kCloseUpDistance + Vector3{ 0.0f, kCloseUpHeight, 0.0f };
	cam->LookAt(playerPos + Vector3{ 0.0f, kLookAtHeight, 0.0f });
	CameraManager::GetInstance().AddCamera(std::move(cam));
	CameraManager::GetInstance().SetActiveCamera("StartCamera");
}

void StageStart::Complete()
{
	isComplete_ = true;
}

void StageStart::Update()
{
	// スタート処理が終わってたら何もせずにreturn
	if (isComplete_) return;

	// スペースを押したらスキップ
	if (Input::GetInstance().IsConnected()) {
		if (Input::GetInstance().PushButton(PadNumber::ButtonA)) {
			CameraManager::GetInstance().SetActiveCamera("GameCamera", 0.3f);
			Complete();
			return;
		}
	} else if (Input::GetInstance().TriggerKey(DIK_SPACE) || Input::GetInstance().TriggerKey(DIK_R)) {
		CameraManager::GetInstance().SetActiveCamera("GameCamera", 0.3f);
		Complete();
		return;
	}

	// 「シーン遷移が終わっていないか」「カメラの移動が行われているか」で早期リターン
	if (!TransitionManager::GetInstance().IsFinished() || CameraManager::GetInstance().IsTransition()) return;

	// アップからゲームカメラへ引いていき、そのままゲーム開始
	if (CameraManager::GetInstance().GetActiveCamera()->name_ == "StartCamera") {
		CameraManager::GetInstance().SetActiveCamera("GameCamera", kPullBackTime);
		return;
	}

	// 引きが終わった（アクティブがGameCameraになった）ので完了
	Complete();
}
