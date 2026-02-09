#pragma once
#include <memory>
#include "GameObject/Character/Player/Controller/PlayerInput.h"

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
private:
	// プレイヤーの入力制御
	std::unique_ptr<PlayerInput> playerInput_ = nullptr;
	// カメラの入力制御フラグ
	bool canPlayerMove_ = false;
};

