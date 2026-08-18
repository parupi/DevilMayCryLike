#pragma once
#include "GameObject/UI/Common/MenuNavigator.h"

#include <Graphics/Rendering/Sprite/Sprite.h>
#include <Graphics/Text/TextLabel.h>
#include <array>
#include <cstdint>

class TrainingController;

/// <summary>
/// トレーニングルームの設定メニュー。
///
/// 戦う相手の種類・行動・無敵などをここで切り替える。
/// タイトルでは種類を選ばずに部屋へ入るので、**敵の設定はここが唯一の入り口**
/// （F1〜F8 のショートカットは同じ操作への近道として残してある）。
///
/// 触る先はすべて TrainingController なので、このクラスは値を持たない。
/// 開いている間はシーンの時間が止まる（GameSceneStateTrainingMenu が受け持つ）ので、
/// 演出は実時間（GetUnscaledDeltaTime）で進める。
///
/// スプライトは SpriteManager が生成順に描くので、TrainingHUD より後に作ること
/// （後から作ったものが手前に出る）
/// </summary>
class TrainingMenu
{
public:
	/// <summary>選ばれた結果。GameSceneStateTrainingMenu が毎フレーム見る</summary>
	enum class Result {
		None,
		Close, ///< ゲームへ戻る
	};

	void Initialize();

	/// <summary>メニューを開く</summary>
	void Open();
	/// <summary>メニューを閉じる（ゲームへ戻るときに GameSceneStateTrainingMenu が呼ぶ）</summary>
	void Close();

	/// <summary>
	/// 毎フレーム呼ぶ。閉じるアニメを進めるため、開いていないフレームでも呼ぶこと
	/// </summary>
	/// <param name="controller">操作の対象。null なら表示だけ進める</param>
	void Update(TrainingController* controller);

	Result GetResult() const { return result_; }

	/// <summary>まだ画面に映っているか（フェード中を含む）</summary>
	bool IsVisible() const { return state_ != State::Hidden; }

private:
	/// <summary>行。並び順がそのまま画面の上からの順番になる</summary>
	enum class Row {
		Enemy,            ///< 戦う相手の種類
		Behavior,         ///< 相手にどこまで行動を許すか
		PlayerInvincible,
		EnemyInvincible,
		AutoRespawn,      ///< 倒したあと自動で出し直すか
		Hud,              ///< 状態表示の ON/OFF

		// ここから下は値を持たない、決定で走る行
		Respawn,
		Reset,
		Back,

		Count,
	};
	static constexpr int32_t kRowCount = static_cast<int32_t>(Row::Count);

	enum class State {
		Hidden,
		Appearing,
		Shown,
		Closing,
	} state_ = State::Hidden;

	/// <summary>1行ぶんの表示要素。決定で走る行は value を使わない</summary>
	struct RowSprites {
		TextLabel* label = nullptr;
		TextLabel* value = nullptr; ///< "GRUNT" や "ON" / "OFF"
	};

	/// <summary>その行が「決定で走る行」かどうか</summary>
	static bool IsActionRow(Row row) { return row >= Row::Respawn; }

	// 左右入力で選択中の行の値を動かす
	void ChangeValue(TrainingController& controller, int32_t direction);
	// 決定で選択中の行を実行する。値の行は1つ先へ進める
	void Decide(TrainingController& controller);
	// 見た目（選択位置・値・アルファ）を今の状態に合わせる
	void Refresh(const TrainingController* controller);

	MenuNavigator navigator_{};

	std::array<RowSprites, kRowCount> rows_{};
	Sprite* backdrop_ = nullptr;
	TextLabel* title_ = nullptr;
	TextLabel* description_ = nullptr; ///< 選択中の敵の説明
	Sprite* highlight_ = nullptr;      ///< 選択中の行の後ろに敷く加算グロー
	Sprite* cursor_ = nullptr;         ///< 選択中の行を指す矢印
	TextLabel* hint_ = nullptr;

	Result result_ = Result::None;

	int32_t selectedIndex_ = 0;
	float alpha_ = 0.0f;
	/// <summary>選択中の行の明滅に使う経過時間</summary>
	float pulseTimer_ = 0.0f;

	// ==========================
	// レイアウト
	// ==========================
	static constexpr float kRowStartY = 186.0f;
	static constexpr float kRowSpacing = 46.0f;
	static constexpr float kLabelX = 372.0f;
	static constexpr float kValueX = 748.0f;
	static constexpr float kCursorX = 334.0f;
	static constexpr float kFontSize = 26.0f;
	static constexpr float kDescriptionY = 604.0f;

	static constexpr float kFadeTime = 0.22f;
	/// <summary>暗幕の濃さ。設定を読ませる場なので、後ろの戦闘は見えなくてよい</summary>
	static constexpr float kBackdropAlpha = 0.9f;
	/// <summary>選ばれていない行の暗さ</summary>
	static constexpr float kDimBrightness = 0.45f;
};
