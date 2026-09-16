#include "PlayerStateClear.h"

#include "Audio/GameSoundLibrary.h"
#include "Audio/SoundManager.h"

void PlayerStateClear::Enter(Player&)
{
	// 勝利ポーズ（拍手モーション）の音。
	// クリア演出はここからしか始まらないので、Enter に置けば二重に鳴らない
	SoundManager::GetInstance().PlaySE(GameSound::kPlayerClap, 0.7f);
}

void PlayerStateClear::Update(Player&, float)
{
}

void PlayerStateClear::Exit(Player&)
{
}

void PlayerStateClear::ExecuteCommand(Player&, const PlayerCommand&)
{
}
