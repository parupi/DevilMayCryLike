#pragma once
#include <stdint.h>
#include <string>
#include <Graphics/Rendering/Sprite/AnimatedSprite.h>

class TextLabel;

/// <summary>
/// 1項目ぶんの説明文。操作の表記はパッドとキーボードで出し分ける
/// </summary>
struct TutorialText {
	std::string padCommand;      // パッドのときの操作（例: "X → X → X"）
	std::string keyboardCommand; // キーボードのときの操作（例: "K → K → K"）
	std::string title;           // 何の操作か（例: "コンボA"）
};

class Tutorial {
public:
	Tutorial() = default;
	~Tutorial() = default;
	// 初期化（name は表示する GIF のファイル名）
	void Initialize(const std::string& name, const TutorialText& text, uint32_t maxCounter);
	// 更新
	void Update();
	// 起動
	void Start();
	// 終了
	void End();
	// 進行度を進める
	bool StepTutorial();
	// 表示が完全に消えているか（フェードアウトが終わったか）
	bool IsInactive() const { return state_ == State::Inactive; }
private:
	// 説明文の表記を入力機器に合わせ、絵と同じ濃さでフェードさせる
	void UpdateText(float alpha);

	// チュートリアルの状態
	enum class State {
		Inactive, // 非アクティブな状態
		Start, // 開始中
		Active, // アクティブな状態
		End, // 終了中
	}state_ = State::Inactive;

	// スプライト
	AnimatedSprite* tutorialImage = nullptr;
	// 説明文。1行目が操作、2行目が何の操作か
	TextLabel* commandLabel_ = nullptr;
	TextLabel* titleLabel_ = nullptr;
	TutorialText text_;
	// 進行度のカウンター
	uint32_t counter_ = 0;
	// カウンターの最大値（チュートリアルの種類ごとにInitializeで指定）
	uint32_t maxCounter_ = 3;
};