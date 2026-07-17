#include "TutorialSystem.h"

namespace {
	// チュートリアルごとの設定（表示画像名と完了に必要な回数）
	struct TutorialConfig {
		std::string imageName;
		uint32_t maxCounter;
	};
}

void TutorialSystem::Initialize() {
	// 装飾クラスの生成
	decoration_ = std::make_unique<TutorialDecoration>();
	decoration_->Initialize();

	// チュートリアルの種類ごとの、表示画像と完了に必要な回数
	static const std::unordered_map<TutorialState, TutorialConfig> kTutorialConfigs = {
		{ TutorialState::Move,           { "PlayerWalk", 50 } }, // 120フレーム移動し続けたら完了
		{ TutorialState::Jump,           { "PlayerJump", 1 } }, // 1度ジャンプしたら完了
		{ TutorialState::AttackA,        { "AttackA", 3 } },   // 攻撃Aを3回当てたら完了
		{ TutorialState::AttackB,        { "AttackB", 2 } },   // 攻撃Bを3回当てたら完了
		{ TutorialState::LockOn,         { "LockOn", 1 } },   // ロックオンを1回行ったら完了
		{ TutorialState::RoundUpAttack,  { "RoundUp", 2 } },   // 切り上げ攻撃を3回当てたら完了
	};

	// チュートリアルの初期化処理
	for (int i = 0; i < static_cast<int>(TutorialState::Count); ++i) {
		TutorialState state = static_cast<TutorialState>(i);
		const TutorialConfig& config = kTutorialConfigs.at(state);
		tutorials_[state] = std::make_unique<Tutorial>();
		tutorials_[state]->Initialize(config.imageName, config.maxCounter);
	}
	// 最初のチュートリアルを設定
	currentTutorial_ = tutorials_[TutorialState::Move].get();
}

void TutorialSystem::Update() {
	// 全チュートリアルを更新する
	// （フェードアウト中に次のチュートリアルが完了しても、消えきるまで更新が途切れないようにするため）
	for (auto& [state, tutorial] : tutorials_) {
		tutorial->Update();
	}

	// 装飾の更新
	decoration_->Update();
}

void TutorialSystem::StartTutorial(TutorialState state) {
	// 背景マスクの表示は、既に表示中でなければ行う（連続再生時に演出をやり直さないため）
	if (decoration_->IsInactive()) {
		decoration_->Start();
	}

	// チュートリアルの状態を設定
	state_ = state;
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

void TutorialSystem::StepTutorial(TutorialState state) {
	// 現在表示中のチュートリアルと関係ないイベントは無視する
	if (state != state_) {
		return;
	}

	// 進行度が一定以上なら次のチュートリアルへ自動的に進める
	if (currentTutorial_->StepTutorial()) {
		AdvanceTutorial();
	}
}

void TutorialSystem::AdvanceTutorial() {
	// 現在のチュートリアルの表示のみ終了させる（背景マスクはここでは触らない）
	// フェードアウトはUpdateで全チュートリアルを更新しているため、消えきるまで自動で進む
	currentTutorial_->End();

	// enumの宣言順を進行順として扱い、次のチュートリアルへ進む
	int nextState = static_cast<int>(state_) + 1;
	if (nextState < static_cast<int>(TutorialState::Count)) {
		// 続けて次のチュートリアルを開始する（背景マスクは維持したまま）
		StartTutorial(static_cast<TutorialState>(nextState));
	} else {
		// 最後まで完了したら背景マスクもフェードアウトさせる
		decoration_->End();
		// 全チュートリアル完了。チュートリアル用の敵などが参照する
		isAllFinished_ = true;
	}
}