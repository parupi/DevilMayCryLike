#pragma once
#include <memory>
#include "GameObject/Character/Player/Controller/PlayerInput.h"

// 
class InputRouter
{
public:
	InputRouter() = default;
	~InputRouter() = default;

	void Initialize(Input* input);

	void Update();

	PlayerInput* GetPlayerInput() { return playerInput_.get(); }

private:
	// プレイヤーの入力制御
	std::unique_ptr<PlayerInput> playerInput_ = nullptr;
	// カメラの入力制御

};

