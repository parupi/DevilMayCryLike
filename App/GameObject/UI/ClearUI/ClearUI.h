#pragma once
#include <array>
#include <cstdint>
#include <string>

#include "Math/Vector2.h"
#include "Math/Vector4.h"

class Sprite;
class TextLabel;
class ClearSequence;
class MenuNavigator;

/// <summary>
/// クリア画面の UI（クリアシーン演出仕様書 2・4・5章）。
///
/// 画面の左に STAGE CLEAR → TOTAL SCORE（カウントアップ）→ RANK を時間差で出し、
/// 最後に下へ RETRY / TITLE のメニューを出す。いつ何を出すかは ClearSequence の時刻で決まり、
/// このクラスは「その時刻ならこう見える」を毎フレーム作り直すだけ（巻き戻しても同じ見た目になる）。
/// SE だけは時刻をまたいだ瞬間に1回鳴らす。
///
/// 文字はフォントから描く。STAGE CLEAR とランクの文字は大きく出すので、
/// 大きめに焼いた専用のフォント（kDisplayFont）を使う。
/// </summary>
class ClearUI
{
public:
	enum class MenuResult {
		None,
		Retry,
		Title,
	};

	/// <summary>使う絵とフォントを読む。Initialize より前に1回</summary>
	static void LoadResources();

	void Initialize();

	/// <summary>表示する結果。ランクは StylishScoreManager の表記（D / C / B / A / S / SS / SSS）</summary>
	void SetResult(int32_t score, const std::string& rank);

	/// <summary>見た目を時刻に合わせる。毎フレーム呼ぶ</summary>
	void Update(const ClearSequence& sequence, float deltaTime);

	/// <summary>メニューの操作。メニューが出てから呼ぶ。決定されたらその結果を返す</summary>
	MenuResult UpdateMenu(const MenuNavigator& navigator);

	/// <summary>ランクの発光・粒子・衝撃波を最初からやり直す（デバッグの再生用）</summary>
	void ResetEffects();

private:
	/// <summary>ランクごとの見た目（仕様書 5章）</summary>
	struct RankStyle {
		Vector4 color;
		float glow;        // 後ろの光の強さ（0〜1）
		int sparkCount;    // 飛び散る光の粒の数
		bool shockwave;    // 衝撃波のリングを出すか
		bool high;         // 高ランク専用の演出（SE・光の脈動）
	};
	static RankStyle StyleOf(const std::string& rank);

	/// <summary>光の粒1つ</summary>
	struct Spark {
		Sprite* sprite = nullptr;
		Vector2 position{};
		Vector2 velocity{};
		float life = 0.0f;
		float maxLife = 0.0f;
		float size = 0.0f;
	};

	void UpdateStageClear(const ClearSequence& sequence);
	void UpdateScore(const ClearSequence& sequence, float deltaTime);
	void UpdateRank(const ClearSequence& sequence, float deltaTime);
	void UpdateMenuVisual(const ClearSequence& sequence);
	void SpawnSparks();

	static void SetLabelAlpha(TextLabel* label, const Vector4& color, float alpha);
	static std::string FormatScore(int32_t score);

	// ── STAGE CLEAR ──
	TextLabel* stageClear_ = nullptr;
	Sprite* stageClearGlow_ = nullptr;
	Sprite* stageClearLine_ = nullptr;
	float stageClearWidth_ = 0.0f;

	// ── スコア ──
	TextLabel* scoreCaption_ = nullptr;
	TextLabel* scoreValue_ = nullptr;
	int32_t targetScore_ = 0;
	int32_t shownScore_ = -1;
	float tickTimer_ = 0.0f;
	bool scoreFinished_ = false;

	// ── ランク ──
	TextLabel* rankCaption_ = nullptr;
	TextLabel* rankLetter_ = nullptr;
	Sprite* rankGlow_ = nullptr;
	std::array<Sprite*, 2> rings_{};
	std::array<Spark, 40> sparks_{};
	std::string rank_ = "D";
	RankStyle style_{};
	Vector2 rankCenter_{};
	float rankEffectTime_ = -1.0f; // ランクの演出が始まってからの秒数（始まる前は負）

	// ── メニュー ──
	std::array<TextLabel*, 2> menuItems_{};
	Sprite* menuCursor_ = nullptr;
	int selected_ = 0;
	float menuAppear_ = 0.0f;
	float cursorX_ = 0.0f;
	float time_ = 0.0f;
};
