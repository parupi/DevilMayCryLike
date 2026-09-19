#pragma once
#include <stdint.h>
#include <string>

class TextLabel;

/// <summary>
/// 1項目ぶんの説明文。操作の表記はパッドとキーボードで出し分ける
/// </summary>
struct TutorialText {
	std::string padCommand;      // パッドのときの操作（例: "X → X → X"）
	std::string keyboardCommand; // キーボードのときの操作（例: "K → K → K"）
	std::string title;           // 何の操作か（例: "コンボA"）
};

/// <summary>
/// 1項目ぶんのチュートリアル表示。以前は GIF ＋ 説明文だったが、
/// 画面を覆う面積とVRAMを減らすため **文字だけ** にしてある
/// </summary>
class Tutorial {
public:
	Tutorial() = default;
	~Tutorial() = default;
	// 初期化（id はスプライト名を分けるための識別子）
	void Initialize(const std::string& id, const TutorialText& text, uint32_t maxCounter);
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
	// 説明文の表記を入力機器に合わせ、今の濃さでフェードさせる
	void UpdateText(float alpha);

	// チュートリアルの状態
	enum class State {
		Inactive, // 非アクティブな状態
		Start, // 開始中
		Active, // アクティブな状態
		End, // 終了中
	}state_ = State::Inactive;

	// 説明文。1行目が操作、2行目が何の操作か
	TextLabel* commandLabel_ = nullptr;
	TextLabel* titleLabel_ = nullptr;
	TutorialText text_;
	// 表示の濃さ。GIF をやめたので、フェードの進行はここが持つ
	float alpha_ = 0.0f;
	// 進行度のカウンター
	uint32_t counter_ = 0;
	// カウンターの最大値（チュートリアルの種類ごとにInitializeで指定）
	uint32_t maxCounter_ = 3;
};
