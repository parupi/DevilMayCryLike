#pragma once
#include <Graphics/Rendering/Sprite/Sprite.h>
#include <Graphics/Text/TextLabel.h>
#include <string>

class MenuNavigator;

/// <summary>
/// 「本当にいいですか？」の確認ダイアログ。
///
/// 取り返しのつかない操作（アプリの終了、やり直し、タイトルへ戻る）の手前に挟む。
/// 事故を防ぐため、開いたときの選択は必ず NO 側から始まる
/// </summary>
class ConfirmDialog
{
public:
	enum class Answer {
		None, ///< まだ決まっていない
		Yes,
		No,
	};

	/// <param name="idPrefix">スプライト名の重複を避けるための接頭辞</param>
	void Initialize(const std::string& idPrefix, SpriteLayer layer);

	/// <summary>問いかけの文言を指定して開く</summary>
	void Open(const std::string& message);
	/// <summary>閉じる。YES のあとも出したままにしたい場合は呼ばない</summary>
	void Close();

	/// <summary>
	/// 入力と表示を1フレーム進める。決まったフレームだけ Yes / No を返す。
	/// 返したあとも開いたままなので、閉じるかどうかは持ち主が決める
	/// </summary>
	Answer Update(const MenuNavigator& navigator);

	/// <summary>開いているか（フェード中は含まない）</summary>
	bool IsOpen() const { return isOpen_; }
	/// <summary>まだ画面に映っているか</summary>
	bool IsVisible() const { return alpha_ > 0.0f; }

private:
	Sprite* backdrop_ = nullptr;
	TextLabel* message_ = nullptr;
	TextLabel* yes_ = nullptr;
	TextLabel* no_ = nullptr;

	bool isOpen_ = false;
	/// <summary>0 = YES / 1 = NO</summary>
	int32_t selectedIndex_ = 1;
	float alpha_ = 0.0f;

	static constexpr float kFadeTime = 0.18f;
	static constexpr float kBackdropAlpha = 0.85f;
	static constexpr float kDimBrightness = 0.45f;
	static constexpr float kMessageY = 320.0f;
	static constexpr float kAnswerY = 420.0f;
	static constexpr float kAnswerOffsetX = 110.0f;
};
