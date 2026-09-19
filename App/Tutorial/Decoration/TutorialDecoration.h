#pragma once
#include <cstdint>

class Sprite;

/// <summary>
/// チュートリアルの説明文を載せる装飾パネル。
///
/// 黒い矩形をそのまま敷くと画面から浮くので、**左右（と上下）が透けていくパネル**＋
/// **上下の仕切り線**＋**四隅の飾り**という Hollow Knight 風の組み方にしている。
/// 絵は1枚も持たず、パネルと四隅の飾りは Initialize でピクセルを組み立てて作る。
/// </summary>
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
	// 今の出具合（progress_）を各スプライトへ反映する
	void Apply();

	// チュートリアルの状態
	enum class State {
		Inactive, // 非アクティブな状態
		Start, // 開始中
		Active, // アクティブな状態
		End, // 終了中
	}state_ = State::Inactive;

	// 出具合。0で完全に消えていて、1で出きっている。
	// パネルの濃さ・仕切り線のディゾルブ・四隅の濃さを全部これ1つから作る
	float progress_ = 0.0f;

	Sprite* panel_ = nullptr;
	Sprite* upDivider_ = nullptr;
	Sprite* underDivider_ = nullptr;
	// 四隅の飾り。左上・右上・左下・右下の順
	static constexpr int32_t kCornerCount = 4;
	Sprite* corners_[kCornerCount] = {};
};
