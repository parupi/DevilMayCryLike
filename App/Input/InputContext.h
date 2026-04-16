#pragma once
#include <memory>
#include "GameObject/Character/Player/Controller/PlayerInput.h"
#include "LockOnInput.h"
#include "CameraInput.h"

// 現在のシーンや状況に応じた入力の受付状態を管理する
class InputContext
{
public:
	InputContext() = default;
	~InputContext() = default;
	// 初期化
	void Initialize(Input* input);
	// 更新
	void Update();
	// プレイヤーの入力を取得
	PlayerInput* GetPlayerInput() { return playerInput_.get(); }
	// プレイヤーの入力フラグを設定
	void SetCanPlayerMove(bool canPlayerMove) { canPlayerMove_ = canPlayerMove; }
	// ロックオン用の入力を取得
	LockOnInput* GetLockOnInput() { return lockOnInput_.get(); }
	// ロックオンの入力フラグを設定
	void SetCanLockOn(bool canLockOn) { canLockOn_ = canLockOn; }
	// カメラ用の入力を取得
	CameraInput* GetCameraInput() { return cameraInput_.get(); }
	// カメラの入力フラグを設定
	void SetCanCameraMove(bool canCameraMove) { canCameraMove_ = canCameraMove; }
private:
	// プレイヤーの入力制御
	std::unique_ptr<PlayerInput> playerInput_ = nullptr;
	// プレイヤーの入力制御フラグ
	bool canPlayerMove_ = false;
	// ロックオンの入力制御
	std::unique_ptr<LockOnInput> lockOnInput_ = nullptr;
	// ロックオンの入力制御
	bool canLockOn_ = false;
	// カメラの入力制御
	std::unique_ptr<CameraInput> cameraInput_ = nullptr;
	// カメラの入力制御フラグ
	bool canCameraMove_ = false;
};

