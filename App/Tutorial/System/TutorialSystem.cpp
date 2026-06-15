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

	// 装飾クラスの生成
	decoration_ = std::make_unique<TutorialDecoration>();
	decoration_->Initialize();
}

void TutorialSystem::Update() {
	// 現在のチュートリアルの更新処理
	currentTutorial_->Update();
	// 装飾の更新
	decoration_->Update();
}

void TutorialSystem::StartTutorial(TutorialState state) {
	// 背景マスクの表示など、チュートリアル開始時の共通処理をここに記述
	decoration_->Start();

	// チュートリアルの状態を設定
	currentTutorial_ = tutorials_[state].get();
	// 番外でなければ
	if (state < TutorialState::Count) {
		// 最初のチュートリアルを開始
		currentTutorial_->Start();
	}
}

void TutorialSystem::EndTutorial() {
	// 背景マスクの非表示など、チュートリアル終了時の共通処理をここに記述
	decoration_->End();
	
	// 現在のチュートリアルを終了
	currentTutorial_->End();
}

void TutorialSystem::StepTutorial() {
	// 進行度が一定以上なら終了
	if (currentTutorial_->StepTutorial()) {
		EndTutorial();
	}
}