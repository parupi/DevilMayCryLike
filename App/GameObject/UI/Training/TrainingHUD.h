#pragma once
#include <array>
#include <cstdint>

class Player;
class Sprite;
class TextLabel;
class TrainingController;

/// <summary>
/// トレーニングルームの状態表示。
///
/// 敵のステート名・再生中のクリップ・HP、与えたダメージ、プレイヤーの状態、
/// スタイルスコア、今の設定と操作キーを画面左に並べる。
/// 挙動を詰めているときに「今どの行動に入ったか」を目で追えるようにするのが目的なので、
/// 見た目より読み取りやすさを優先して素の文字だけで組んでいる。
///
/// スプライトは SpriteManager が生成順に描くので、
/// ポーズの暗幕やゲームオーバーより先に作ること（後から作ったものが手前に出る）
/// </summary>
class TrainingHUD
{
public:
	void Initialize();

	/// <summary>毎フレーム呼ぶ。controller が非表示設定なら勝手に消える</summary>
	void Update(const TrainingController& controller, Player* player);

	/// <summary>強制的に消す（ポーズ・ゲームオーバー中など）</summary>
	void Hide();

private:
	/// <summary>行。並び順がそのまま画面の上からの順番になる</summary>
	enum class Row {
		Title,     ///< "TRAINING" と選択中の敵
		EnemyHp,   ///< 敵のHP
		EnemyState,///< 敵のステートと再生中のクリップ
		Damage,    ///< 与えたダメージの合計・ヒット数
		PlayerHp,  ///< プレイヤーのHP
		Score,     ///< ランク・スタイルポイント・コンボ
		Settings,  ///< 無敵などのスイッチの状態
		KeysA,     ///< 操作キー（1行目）
		KeysB,     ///< 操作キー（2行目）

		Count,
	};
	static constexpr int32_t kRowCount = static_cast<int32_t>(Row::Count);

	TextLabel* Label(Row row) { return rows_[static_cast<int32_t>(row)]; }
	void SetRow(Row row, const char* text, float brightness = 1.0f);

	std::array<TextLabel*, kRowCount> rows_{};
	Sprite* backdrop_ = nullptr; ///< 明るい床の上でも読めるようにする薄い下敷き

	bool visible_ = false;

	// ==========================
	// レイアウト（画面 1280x720、左上）
	// ==========================
	static constexpr float kLeftX = 22.0f;
	/// <summary>HPのハートが y=18〜82 を占めるので、その下から始める</summary>
	static constexpr float kTopY = 108.0f;
	static constexpr float kRowSpacing = 22.0f;
	static constexpr float kFontSize = 17.0f;
	static constexpr float kBackdropWidth = 470.0f;
};
