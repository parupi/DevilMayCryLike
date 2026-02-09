#include "PlayerStateMachine.h"
#include "GameObject/Character/Player/Controller/PlayerInput.h"
#include <imgui.h>

using namespace std;

PlayerStateMachine::~PlayerStateMachine()
{
	states_.clear();
	currentState_ = nullptr;
}

void PlayerStateMachine::SetFirstState(string stateName, unique_ptr<PlayerStateBase> state)
{
	// 既にステートがセットされていたらreturn
	if (currentState_) return;

	states_[stateName] = move(state);
	currentState_ = states_[stateName].get();
}

void PlayerStateMachine::AddState(string stateName, unique_ptr<PlayerStateBase> state)
{
	// 既に同じ名前のstateがあれば追加しない
	if (states_[stateName]) return;

	states_[stateName] = move(state);
}

void PlayerStateMachine::UpdateCurrentState(Player& player, float deltaTime)
{
	// 現在のステートを更新
	if (currentState_) {
		currentState_->Update(player, deltaTime);
	}
}

void PlayerStateMachine::ChangeState(Player& player, string stateName)
{
	currentState_->Exit(player);
	auto it = states_.find(stateName);
	if (it != states_.end()) {
		currentState_ = it->second.get();
		currentState_->Enter(player);
	}
}

void PlayerStateMachine::ExecuteCommand(Player& player, const PlayerCommand& command)
{
	// 現在のステートに入力を送る
	if (currentState_) {
		currentState_->ExecuteCommand(player, command);
	}
}

void PlayerStateMachine::DebugGui()
{
	ImGui::Begin("Player");
	ImGui::Text("Current State: %s", currentState_->GetDebugName());
	ImGui::End();
}
