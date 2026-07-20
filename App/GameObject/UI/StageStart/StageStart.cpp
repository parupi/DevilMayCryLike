#include "StageStart.h"
#include <memory>
#include <World3D/Camera/CameraManager.h>
#include "World3D/Camera/BaseCamera.h"
#include <World3D/Object/Object3d.h>
#include "GameObject/Camera/GameCamera.h"
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
	if (auto* player = Object3dManager::GetInstance().FindObject("Player")) {
		playerPos = player->GetWorldTransform()->GetTranslation();
		// 初回Update前でワールド行列が未計算のため、回転クォータニオンから前方向を求める
		Vector3 forward = RotateVector({ 0.0f, 0.0f, 1.0f }, player->GetWorldTransform()->GetRotation());
		forward.y = 0.0f; // 水平方向のみ
		if (Length(forward) > 0.001f) {
			playerForward_ = Normalize(forward);
		}
	}

	auto cam = std::make_unique<BaseCamera>("StartCamera");
	cam->GetTranslate() = playerPos - playerForward_ * kCloseUpDistance + Vector3{ 0.0f, kCloseUpHeight, 0.0f };
	cam->LookAt(playerPos + Vector3{ 0.0f, kLookAtHeight, 0.0f });
	CameraManager::GetInstance().AddCamera(std::move(cam));
	CameraManager::GetInstance().SetActiveCamera("StartCamera");
}

void StageStart::Complete()
{
	isComplete_ = true;
}

void StageStart::PrepareGameCamera()
{
	// CameraManager は SetActiveCamera を呼んだ時点の相手カメラの位置を補間先として記録し、
	// 補間中は相手カメラのUpdateを回さない。GameCameraは一度もアクティブになっていないので
	// 生成時の固定座標のままであり、先に追従位置へ移動させておかないと
	// 「プレイヤーの背後へ引く」ではなく、ステージとは無関係な座標へ飛んでしまう。
	if (auto* cam = dynamic_cast<GameCamera*>(CameraManager::GetInstance().FindCamera("GameCamera"))) {
		cam->SnapToFollow(playerForward_);
	}
}

void StageStart::Update()
{
	// スタート処理が終わってたら何もせずにreturn
	if (isComplete_) return;

	// スペースを押したらスキップ
	if (Input::GetInstance().IsConnected()) {
		if (Input::GetInstance().PushButton(PadNumber::ButtonA)) {
			PrepareGameCamera();
			CameraManager::GetInstance().SetActiveCamera("GameCamera", 0.3f);
			Complete();
			return;
		}
	} else if (Input::GetInstance().TriggerKey(DIK_SPACE) || Input::GetInstance().TriggerKey(DIK_R)) {
		PrepareGameCamera();
		CameraManager::GetInstance().SetActiveCamera("GameCamera", 0.3f);
		Complete();
		return;
	}

	// 「シーン遷移が終わっていないか」「カメラの移動が行われているか」で早期リターン
	if (!TransitionManager::GetInstance().IsFinished() || CameraManager::GetInstance().IsTransition()) return;

	// アップからゲームカメラへ引いていき、そのままゲーム開始
	if (CameraManager::GetInstance().GetActiveCamera()->name_ == "StartCamera") {
		PrepareGameCamera();
		CameraManager::GetInstance().SetActiveCamera("GameCamera", kPullBackTime);
		return;
	}

	// 引きが終わった（アクティブがGameCameraになった）ので完了
	Complete();
}
