#pragma once
#include <string>
#include <GameObject/Character/Player/State/PlayerStateBase.h>
#include <unordered_map>

// プレイヤーのステートの管理するクラス
class PlayerStateMachine
{
public:
	PlayerStateMachine() = default;
	~PlayerStateMachine();

	// 最初のステートを設定
	void SetFirstState(std::string stateName, std::unique_ptr<PlayerStateBase> state);
	// ステートの追加
	void AddState(std::string stateName, std::unique_ptr<PlayerStateBase> state);
	// 現在のステートの更新
	void UpdateCurrentState(Player& player, float deltaTime);
	// ステートの変更
	void ChangeState(Player& player, std::string stateName);
	// 現在のステートを取得
	PlayerStateBase* GetCurrentState() { return currentState_; }
	// 入力を取得
	void ExecuteCommand(Player& player, const PlayerCommand& command);
	// デバッグ情報を出力
	void DebugGui();
private:
	// ステート名とステートインスタンスのマップ
	std::unordered_map<std::string, std::unique_ptr<PlayerStateBase>> states_; 
	// 現在のステート
	PlayerStateBase* currentState_ = nullptr; 
};

