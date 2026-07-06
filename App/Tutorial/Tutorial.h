#pragma once
#include <stdint.h>
#include <Graphics/Rendering/Sprite/AnimatedSprite.h>

class Sprite;

class Tutorial {
public:
	Tutorial() = default;
	~Tutorial() = default;
	// 初期化
	void Initialize(const std::string& name, uint32_t maxCounter);
	// 更新
	void Update();
	// 起動
	void Start();
	// 終了
	void End();
	// チュートリアルが終了中かどうか
	bool IsEnding() const { return state_ == State::End; }
	// 進行度を進める
	bool StepTutorial();
private:
	// チュートリアルの状態
	enum class State {
		Inactive, // 非アクティブな状態
		Start, // 開始中
		Active, // アクティブな状態
		End, // 終了中
	}state_ = State::Inactive;

	// スプライト
	AnimatedSprite* tutorialImage = nullptr;
	Sprite* tutorialText = nullptr;
	// 進行度のカウンター
	uint32_t counter_ = 0;
	// カウンターの最大値（チュートリアルの種類ごとにInitializeで指定）
	uint32_t maxCounter_ = 3;
};