#pragma once
#include <Graphics/Rendering/Sprite/Sprite.h>
#include <Graphics/Text/TextLabel.h>
#include <array>

/// <summary>
/// タイトルの CONTROLS 画面。
///
/// 操作名を左、割り当てを右に並べた一覧を出す。
/// 割り当てはパッドとキーボードで変わるので、接続状況を見て文字列を差し替える。
/// 入力（閉じる操作）は TitleMenu 側が受け持つ
/// </summary>
class ControlsPanel
{
public:
	void Initialize();

	/// <summary>表示を始める</summary>
	void Open();
	/// <summary>閉じ始める</summary>
	void Close();

	void Update();

	/// <summary>まだ画面に映っているか（フェード中を含む）</summary>
	bool IsVisible() const { return state_ != State::Hidden; }

private:
	/// <summary>操作1行ぶん。割り当ての表記はパッドとキーボードで持ち替える</summary>
	struct ControlRow {
		const char* action;
		const char* pad;
		const char* keyboard;
	};

	static const ControlRow kRows[];
	static const int32_t kRowCount;

	enum class State {
		Hidden,
		Appearing,
		Shown,
		Closing,
	} state_ = State::Hidden;

	// 表示中のスプライトと文字へ今のアルファを流し込む
	void ApplyAlpha();

	Sprite* backdrop_ = nullptr;  ///< 文字を読ませるための暗幕
	TextLabel* title_ = nullptr;
	TextLabel* hint_ = nullptr;

	std::array<TextLabel*, 8> actionLabels_{};  ///< 操作名（左寄せ）
	std::array<TextLabel*, 8> bindLabels_{};    ///< 割り当て（右寄せ）
	std::array<Sprite*, 8> separators_{};       ///< 行を区切る細い線

	float alpha_ = 0.0f;

	// ==========================
	// レイアウト
	// ==========================
	static constexpr float kRowStartY = 220.0f;
	static constexpr float kRowSpacing = 52.0f;
	static constexpr float kActionX = 300.0f;
	static constexpr float kBindX = 980.0f;
	static constexpr float kFontSize = 28.0f;

	/// <summary>フェードにかける秒数</summary>
	static constexpr float kFadeTime = 0.22f;
	/// <summary>暗幕の濃さ。タイトルロゴはブルームで相当明るいので、かなり濃くしないと透けてくる</summary>
	static constexpr float kBackdropAlpha = 0.94f;
};
