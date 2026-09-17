#pragma once
#include <Graphics/Rendering/Sprite/Sprite.h>
#include <Graphics/Text/TextLabel.h>
#include <array>

class MenuNavigator;

/// <summary>
/// タイトルの OPTION 画面。
///
/// 音量は SoundManager、それ以外は GameSettings が持っている値を直接触る。
/// どちらも GlobalVariables 上の値なので、閉じるときにまとめてファイルへ書き出す。
/// 開閉の判断（キャンセルで閉じる等）は TitleMenu 側が受け持つ
/// </summary>
class OptionPanel
{
public:
	void Initialize();

	/// <summary>表示を始める</summary>
	void Open();
	/// <summary>閉じ始める。このとき設定をファイルへ保存する</summary>
	void Close();

	void Update(const MenuNavigator& navigator);

	/// <summary>まだ画面に映っているか（フェード中を含む）</summary>
	bool IsVisible() const { return state_ != State::Hidden; }

private:
	/// <summary>項目。並び順がそのまま画面の上からの順番になる</summary>
	enum class Row {
		MasterVolume,
		BgmVolume,
		SeVolume,
		Sensitivity,
		InvertY,
		Tutorial,

		Count,
	};
	static constexpr int32_t kRowCount = static_cast<int32_t>(Row::Count);

	enum class State {
		Hidden,
		Appearing,
		Shown,
		Closing,
	} state_ = State::Hidden;

	/// <summary>1行ぶんの表示要素。ON/OFF の行はバーを使わない</summary>
	struct RowSprites {
		TextLabel* label = nullptr;
		TextLabel* value = nullptr;  ///< 数値なら "80"、切り替えなら "ON" / "OFF"
		Sprite* barBack = nullptr;
		Sprite* barFill = nullptr;
	};

	// 左右入力で選択中の行の値を動かす
	void ChangeValue(int32_t direction);
	// 見た目（選択位置・値・アルファ）を今の状態に合わせる
	void Refresh();
	// 数値の行の表示を更新する。ratio はバーの伸び具合、number は表示する数
	void RefreshNumberRow(RowSprites& row, float ratio, int32_t number, float brightness);
	// ON/OFF の行の表示を更新する
	void RefreshToggleRow(RowSprites& row, bool value, float brightness);

	// その行が ON/OFF の行かどうか
	static bool IsToggleRow(Row row) { return row == Row::InvertY || row == Row::Tutorial; }

	std::array<RowSprites, kRowCount> rows_{};
	Sprite* backdrop_ = nullptr;
	TextLabel* title_ = nullptr;
	Sprite* highlight_ = nullptr;  ///< 選択中の行の後ろに敷く加算グロー
	Sprite* cursor_ = nullptr;     ///< 選択中の行を指す矢印
	TextLabel* hint_ = nullptr;

	int32_t selectedIndex_ = 0;
	float alpha_ = 0.0f;
	/// <summary>選択中の行の明滅に使う経過時間</summary>
	float pulseTimer_ = 0.0f;

	// ==========================
	// レイアウト
	// ==========================
	static constexpr float kRowStartY = 236.0f;
	static constexpr float kRowSpacing = 62.0f;
	static constexpr float kLabelX = 330.0f;
	static constexpr float kValueX = 700.0f;
	static constexpr float kBarWidth = 240.0f;
	static constexpr float kBarHeight = 6.0f;
	/// <summary>数値の右端。ここへ右寄せで置く</summary>
	static constexpr float kValueRightX = 1010.0f;
	static constexpr float kCursorX = 292.0f;
	static constexpr float kFontSize = 28.0f;

	static constexpr float kFadeTime = 0.22f;
	/// <summary>暗幕の濃さ。タイトルロゴはブルームで相当明るいので、かなり濃くしないと透けてくる</summary>
	static constexpr float kBackdropAlpha = 0.94f;
	/// <summary>選ばれていない行の暗さ</summary>
	static constexpr float kDimBrightness = 0.45f;
};
