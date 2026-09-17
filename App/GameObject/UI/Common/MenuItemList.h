#pragma once
#include <Graphics/Rendering/Sprite/Sprite.h>
#include <Graphics/Text/TextLabel.h>
#include <array>
#include <string>
#include <vector>

class MenuNavigator;

/// <summary>
/// 縦に並んだ選択肢。タイトル・ポーズ・ゲームオーバーで同じ見た目と操作にするための共通部品。
///
/// 選択中の項目を明るくし、後ろに加算グローを敷いて、左右から矢印で挟む。
/// 矢印は項目の実寸に合わせて置くので、文字数が違っても間延びしない。
///
/// 「決定されたか」はここでは見ない。持ち主が MenuNavigator の結果と
/// GetSelectedIndex() を組み合わせて判断すること
/// </summary>
class MenuItemList
{
public:
	/// <summary>
	/// 生成する
	/// </summary>
	/// <param name="idPrefix">スプライト名の重複を避けるための接頭辞（"titleMenu" など）</param>
	/// <param name="firstItemCenter">1つ目の項目の中心位置</param>
	/// <param name="spacing">項目どうしの間隔</param>
	void Initialize(const std::string& idPrefix, SpriteLayer layer,
		const std::vector<std::string>& items,
		const Vector2& firstItemCenter, float spacing, float fontSize);

	/// <summary>上下入力で選択を動かす。動いたら true（決定音とは別のSEを鳴らす）</summary>
	bool UpdateSelection(const MenuNavigator& navigator);

	/// <summary>見た目を今の状態に合わせる。alpha が 0 なら消える</summary>
	void Refresh(float alpha, float deltaTime);

	int32_t GetSelectedIndex() const { return selectedIndex_; }
	void SetSelectedIndex(int32_t index);
	int32_t GetItemCount() const { return static_cast<int32_t>(items_.size()); }

	/// <summary>項目の文字を差し替える（表示中に内容が変わるものがあれば使う）</summary>
	void SetItemText(int32_t index, const std::string& text);

private:
	std::vector<TextLabel*> items_;
	Sprite* highlight_ = nullptr;      ///< 選択中の項目の後ろに敷く加算グロー
	std::array<Sprite*, 2> arrows_{};  ///< 選択中の項目を左右から挟む矢印

	int32_t selectedIndex_ = 0;
	Vector2 firstItemCenter_{};
	float spacing_ = 0.0f;
	/// <summary>選択中の項目の明滅に使う経過時間</summary>
	float pulseTimer_ = 0.0f;

	/// <summary>矢印を項目の端からどれだけ離すか</summary>
	static constexpr float kArrowMargin = 34.0f;
	/// <summary>選ばれていない項目の暗さ</summary>
	static constexpr float kDimBrightness = 0.45f;
};
