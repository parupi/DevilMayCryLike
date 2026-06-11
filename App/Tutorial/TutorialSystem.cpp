#include "TutorialSystem.h"

void TutorialSystem::Initialize() {
	// チュートリアルの初期化処理
	for (int i = 0; i < static_cast<int>(TutorialState::Count); ++i) {
		TutorialState state = static_cast<TutorialState>(i);
		tutorials_[state] = std::make_unique<Tutorial>();
		tutorials_[state]->Initialize();
	}
	// 最初のチュートリアルを設定
	currentTutorial_ = tutorials_[TutorialState::Move].get();
}

void TutorialSystem::Update() {
	// 現在のチュートリアルの更新処理
	currentTutorial_->Update();
	//// チュートリアルの切り替えが必要な場合、切り替え処理を行う
	//if (isTutorialChanging_) {
	//	ChangeTutorialUpdate();
	//}
}

void TutorialSystem::StartTutorial(TutorialState state) {
	// 背景マスクの表示など、チュートリアル開始時の共通処理をここに記述

	// チュートリアルの状態を設定
	currentTutorial_ = tutorials_[state].get();
	// 番外でなければ
	if (state < TutorialState::Count) {
		// 最初のチュートリアルを開始
		currentTutorial_->Start();
	}
}

//void TutorialSystem::NextTutorial() {
//	// チュートリアルの状態を次に進める
//	state_ = static_cast<State>(static_cast<int>(state_) + 1);
//
//	// 最後でなければ
//	if (state_ < State::Count) {
//		// チュートリアル切り替えフラグを立てる
//		isTutorialChanging_ = true;
//	}
//}

void TutorialSystem::EndTutorial() {
	// 背景マスクの非表示など、チュートリアル終了時の共通処理をここに記述
	
	// 現在のチュートリアルを終了
	currentTutorial_->End();
	// 次のチュートリアルに進む
	//NextTutorial();
}

//void TutorialSystem::ChangeTutorialUpdate() {
//	// 現在のチュートリアルが終了している場合、次のチュートリアルに進む
//	if (currentTutorial_->IsEnding()) {
//		return; // 現在のチュートリアルがまだ終了していない場合は何もしない
//	}
//
//	// 次のチュートリアルを設定
//	if (state_ < State::Count) {
//		currentTutorial_ = tutorials_[state_].get();
//		// 切り替えフラグをリセット
//		isTutorialChanging_ = false;
//	}
//}
