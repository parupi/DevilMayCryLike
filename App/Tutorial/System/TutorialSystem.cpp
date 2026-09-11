#include "TutorialSystem.h"

namespace {
	// チュートリアルごとの設定（表示画像名・説明文・完了に必要な回数）
	struct TutorialConfig {
		std::string imageName;
		TutorialText text;
		uint32_t maxCounter;
	};
}

void TutorialSystem::Initialize(bool enabled) {
	enabled_ = enabled;

	// 流さないシーン（トレーニング、OPTIONでオフ）では表示物を一切作らない。
	// 作ったうえでアルファ0にする方式だと、消し忘れた1枚が画面左に残るし、
	// 使わない GIF を6本ぶん読み込んでVRAMも食う。
	// TutorialDummy::CanDie() が全チュートリアル完了を条件にしているので、完了扱いにしておく
	if (!enabled_) {
		isFinishing_ = false;
		isAllFinished_ = true;
		return;
	}

	// 装飾クラスの生成
	decoration_ = std::make_unique<TutorialDecoration>();
	decoration_->Initialize();

	// チュートリアルの種類ごとの、表示画像・説明文・完了に必要な回数。
	// 説明文の操作は { パッド, キーボード, 何の操作か } の順。
	// PlayerInput / LockOnInput と攻撃の json（ButtonIndex 2 = Y / J）の割り当てに合わせてあるので、
	// 割り当てを変えたらここも直すこと
	static const std::unordered_map<TutorialState, TutorialConfig> kTutorialConfigs = {
		// 120フレーム移動し続けたら完了
		{ TutorialState::Move,          { "PlayerWalk", { "左スティック", "W A S D", "移動" }, 50 } },
		// 1度ジャンプしたら完了
		{ TutorialState::Jump,          { "PlayerJump", { "Aボタンを押したら", "SPACEキーを押したら", "ジャンプ" }, 1 } },
		// 攻撃Aを3回当てたら完了
		{ TutorialState::AttackA,       { "AttackA", { "Y → Y → Y", "J → J → J", "コンボA" }, 3 } },
		// 攻撃Bを3回当てたら完了
		{ TutorialState::AttackB,       { "AttackB", { "Y → 少し待つ → Y → Y", "J → 少し待つ → J → J", "コンボB" }, 2 } },
		// ロックオンを1回行ったら完了（離すと外れるので長押し）
		{ TutorialState::LockOn,        { "LockOn", { "RB 長押し", "P 長押し", "ロックオン" }, 1 } },
		// 切り上げ攻撃を3回当てたら完了（ロックオン中にスティック／Sキーを手前へ入れて攻撃）
		{ TutorialState::RoundUpAttack, { "RoundUp", { "RB + 左スティック↓ + Y", "P + S + J", "切り上げ攻撃" }, 2 } },
	};

	// チュートリアルの初期化処理
	for (int i = 0; i < static_cast<int>(TutorialState::Count); ++i) {
		TutorialState state = static_cast<TutorialState>(i);
		const TutorialConfig& config = kTutorialConfigs.at(state);
		tutorials_[state] = std::make_unique<Tutorial>();
		tutorials_[state]->Initialize(config.imageName, config.text, config.maxCounter);
	}
	// 最初のチュートリアルを設定
	currentTutorial_ = tutorials_[TutorialState::Move].get();
}

void TutorialSystem::Update() {
	if (!enabled_) return;

	// 全チュートリアルを更新する
	// （フェードアウト中に次のチュートリアルが完了しても、消えきるまで更新が途切れないようにするため）
	for (auto& [state, tutorial] : tutorials_) {
		tutorial->Update();
	}

	// 装飾の更新
	decoration_->Update();

	// 最後のチュートリアルの絵と背景マスクが消えきってから「全部クリア」にする。
	// AdvanceTutorial の時点で完了にしてしまうと、最後の1発が
	// 「チュートリアルを終わらせた攻撃」と「練習台へのとどめ」を兼ねてしまい、
	// まだ手順の絵が出ているうちに TutorialDummy が倒れる（CanDie() がその場で true になるため）
	if (isFinishing_ && currentTutorial_->IsInactive() && decoration_->IsInactive()) {
		isFinishing_ = false;
		isAllFinished_ = true;
	}
}

void TutorialSystem::StartTutorial(TutorialState state) {
	// 流さないシーンでは表示物を作っていないので何もしない。
	// 呼び出し側（GameSceneStatePlay）に条件分岐を持たせないための入口ガード
	if (!enabled_) return;

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
	if (!enabled_) return;

	// 背景マスクの非表示など、チュートリアル終了時の共通処理をここに記述
	decoration_->End();

	// 現在のチュートリアルを終了
	currentTutorial_->End();
}

void TutorialSystem::StepTutorial(TutorialState state) {
	// プレイヤーの各ステートから毎フレーム飛んでくるので、無効なシーンではここで捨てる
	if (!enabled_) return;

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
		// 表示が消えきるまでは「完了」にしない（Update で isAllFinished_ を立てる）。
		// ここで即座に完了にすると、最後の1発でそのまま TutorialDummy が倒れてしまう
		isFinishing_ = true;
	}
}