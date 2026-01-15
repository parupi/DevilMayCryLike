#pragma once
#include <string>
#include <GameObject/Character/Player/State/PlayerStateBase.h>
class PlayerStateMachine
{
public:
	PlayerStateMachine() = default;
	~PlayerStateMachine() = default;
	// ステートの追加
	void AddState(std::string stateName, std::has_unique_object_representations<PlayerStateBase> state);
	// 現在のステートの更新
	void UpdateCurrentState();
	// ステートの変更
	void ChangeState(std::string stateName);

private:

};

