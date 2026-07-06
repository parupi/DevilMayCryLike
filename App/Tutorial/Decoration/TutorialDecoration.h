#pragma once

class Sprite;
// 背景マスクやディバイダ―を表示するクラス
class TutorialDecoration {
public:
	TutorialDecoration() = default;
	~TutorialDecoration() = default;
	// 初期化
	void Initialize();
	// 更新処理
	void Update();
	// 表示開始
	void Start();
	// 表示終了
	void End();
	// 非アクティブな状態かどうか（表示中に再度Startさせないための判定に使う）
	bool IsInactive() const { return state_ == State::Inactive; }
private:
	// チュートリアルの状態
	enum class State {
		Inactive, // 非アクティブな状態
		Start, // 開始中
		Active, // アクティブな状態
		End, // 終了中
	}state_ = State::Inactive;

	Sprite* musk_ = nullptr;
	Sprite* upDivider_ = nullptr;
	Sprite* underDivider_ = nullptr;
};