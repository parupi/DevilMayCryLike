#include "AttackPlayer.h"
#include <imgui.h>
#include <GameObject/Character/Player/Player.h>
#include <GameObject/Character/Player/State/Attack/PlayerStateAttack.h>

void AttackPlayer::SetPlayer(Player* player)
{
	player_ = player;
}

void AttackPlayer::SetAttacks(std::vector<PlayerStateAttack*> attacks)
{
	attacks_ = attacks;
}

void AttackPlayer::Update(float deltaTime)
{
	if (!isPlaying_ || !currentAttack_ || !player_) return;

	if (state_ == PlayState::Playing) {
		float dt = DeltaTime::GetDeltaTime();
		debugTime_ += dt;

		AttackRequestData req = currentAttack_->Update(*player_, deltaTime);

		if (currentAttack_->IsFinished()) {
			Stop();
		}
	}
}

void AttackPlayer::DrawImGui()
{
#ifdef _DEBUG
	if (!ImGui::Begin("Attack Player")) {
		ImGui::End();
		return;
	}

	// 攻撃選択
	std::vector<const char*> names;
	for (auto* atk : attacks_) {
		names.push_back(atk->name_.c_str());
	}

	ImGui::Combo("Attack", &selectedAttackIndex_, names.data(), (int)names.size());

	//if (!isPlaying_) {
	//    if (ImGui::Button("Play")) {
	//        Play();
	//    }
	//} else {
	//    if (ImGui::Button("Stop")) {
	//        Stop();
	//    }
	//}

	//ImGui::Separator();

	//if (currentAttack_) {
	//    AttackData data = currentAttack_->GetAttackData();

	//    ImGui::Text("Time: %.2f / %.2f", debugTime_, data.totalDuration);

	//    float t = debugTime_ / data.totalDuration;
	//    ImGui::ProgressBar(t, ImVec2(-1, 0));

	//    ImGui::Text("Pre: %.2f  Active: %.2f  Post: %.2f",
	//        data.preDelay,
	//        data.attackDuration,
	//        data.postDelay
	//    );
	//}

	//ImGui::End();

	AttackData data = currentAttack_ ? currentAttack_->GetAttackData() : AttackData{};

	// 再生制御
	if (state_ == PlayState::Playing) {
		if (ImGui::Button("Pause")) Pause();
	} else if (state_ == PlayState::Paused) {
		if (ImGui::Button("Resume")) Resume();
	} else {
		if (ImGui::Button("Play")) Play();
	}

	// タイムライン
	if (currentAttack_) {
		ImGui::Separator();

		ImGui::Text("Time %.2f / %.2f", debugTime_, data.totalDuration);

		bool changed = ImGui::SliderFloat(
			"Timeline",
			&seekTime_,
			0.0f,
			data.totalDuration
		);

		// ★ Pause中のみシーク可能
		if (state_ == PlayState::Paused && changed) {
			Seek(seekTime_);
		}

		// フェーズ可視化
		ImGui::Text("Startup  %.2f", data.preDelay);
		ImGui::Text("Active   %.2f", data.attackDuration);
		ImGui::Text("Recovery %.2f", data.postDelay);
	}

	ImGui::End();
#endif
}

void AttackPlayer::Play()
{
	if (!player_ || attacks_.empty()) return;

	currentAttack_ = attacks_[selectedAttackIndex_];

	currentAttack_->UpdateAttackData(); // ★ 重要
	currentAttack_->Enter(*player_);

	isPlaying_ = true;
	debugTime_ = 0.0f;
	state_ = PlayState::Playing;
}

void AttackPlayer::Stop()
{
	if (currentAttack_ && player_) {
		currentAttack_->Exit(*player_);
	}

	currentAttack_ = nullptr;
	isPlaying_ = false;
	debugTime_ = 0.0f;
	state_ = PlayState::Stop;
}

void AttackPlayer::Pause()
{
	if (state_ == PlayState::Playing) {
		state_ = PlayState::Paused;
	}
}

void AttackPlayer::Resume()
{
	if (state_ == PlayState::Paused) {
		state_ = PlayState::Playing;
	}
}

void AttackPlayer::Seek(float time)
{
	if (!currentAttack_ || !player_) return;

	// 再初期化
	currentAttack_->Exit(*player_);
	currentAttack_->UpdateAttackData();
	currentAttack_->Enter(*player_);

	debugTime_ = time;

	currentAttack_->Update(*player_, debugTime_);
}
