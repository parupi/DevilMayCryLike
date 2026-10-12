#pragma once
#include <string>

class Sprite;
class TextLabel;

/// <summary>
/// チュートリアルをスキップする長押しのゲージ（画面右下）。
///
/// 「START 長押し：スキップ」の文字と、その下の横長のバーで、あとどれだけ押せばよいかを見せる。
/// チュートリアル中は押していなくても控えめな濃さで出しておき（スキップできることの案内）、
/// 押している間ははっきり出す。満ちた瞬間は白く光る。
/// 絵のファイルは使わず white.png の矩形を3枚重ねている（枠・溝・中身）。
///
/// スプライトは SpriteManager が生成順に描くので、ポーズの暗幕より先に作ること
/// </summary>
class TutorialSkipGauge
{
public:
	void Initialize();

	/// <summary>
	/// 毎フレーム呼ぶ。
	/// </summary>
	/// <param name="progress">ゲージの進み具合 [0, 1]</param>
	/// <param name="show">押し続けている・スキップが決まった。はっきり出す</param>
	/// <param name="resident">チュートリアル中か。押していなくても案内として控えめに出しておく</param>
	/// <param name="allowed">操作中か。ポーズやゲームオーバー中は false にして即座に消す</param>
	/// <param name="deltaTime">実時間の経過[s]（ヒットストップやスローの影響を受けない値）</param>
	void Update(float progress, bool show, bool resident, bool allowed, float deltaTime);

	/// <summary>スキップが決まった瞬間に呼ぶ。バーを白く光らせる</summary>
	void Flash();

private:
	void Apply();

	Sprite* frame_ = nullptr; ///< 外枠（薄い明色）
	Sprite* track_ = nullptr; ///< 溝（暗い）
	Sprite* fill_ = nullptr;  ///< 中身。左端を基準に幅で進み具合を出す
	TextLabel* label_ = nullptr;

	float progress_ = 0.0f;
	float alpha_ = 0.0f;
	float flash_ = 0.0f;
	bool allowed_ = false;
	/// <summary>パッドの有無で表記を変える。変わったときだけ SetText する</summary>
	std::string labelText_;

	// ==========================
	// レイアウト（画面 1280x720、右下）
	// ==========================
	static constexpr float kBarRight = 1240.0f;
	static constexpr float kBarY = 668.0f;
	static constexpr float kBarWidth = 220.0f;
	static constexpr float kBarHeight = 8.0f;
	static constexpr float kFrameThickness = 2.0f;
	static constexpr float kLabelGap = 18.0f;   ///< バーの上端から文字の中心まで
	static constexpr float kFontSize = 18.0f;

	/// <summary>押していないときの濃さ。案内として読めるが、戦闘の邪魔にならない程度</summary>
	static constexpr float kIdleAlpha = 0.8f;
	static constexpr float kFadeInSpeed = 8.0f;  ///< [1/s]
	static constexpr float kFadeOutSpeed = 4.0f; ///< [1/s]
	static constexpr float kFlashDecay = 2.5f;   ///< [1/s]
};
