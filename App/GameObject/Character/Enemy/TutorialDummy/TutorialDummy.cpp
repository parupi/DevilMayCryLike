#include "TutorialDummy.h"
#include "GameObject/Character/Player/Player.h"

bool TutorialDummy::CanDie() const {
	// プレイヤーやチュートリアルサービスが未接続の場面（チュートリアル外）では
	// 通常の敵と同じく倒せる扱いにする。
	if (!player_) return true;

	TutorialService* tutorial = player_->GetTutorialService();
	if (!tutorial) return true;

	// 全チュートリアルが完了するまでは死亡させない。
	return tutorial->IsAllFinished();
}
