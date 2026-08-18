#pragma once
#include "GameObject/UI/Common/ConfirmDialog.h"
#include "GameObject/UI/Common/ControlsPanel.h"
#include "GameObject/UI/Common/MenuItemList.h"
#include "GameObject/UI/Common/MenuNavigator.h"
#include "GameObject/UI/Common/OptionPanel.h"

#include <Graphics/Text/TextLabel.h>
#include <memory>

/// <summary>
/// タイトルの選択メニュー。
///
/// GAME START / TRAINING / CONTROLS / OPTION / QUIT の5項目を持ち、
/// CONTROLS・OPTION は子パネルへ委譲する。QUIT は誤爆すると
/// アプリが落ちてしまうので、必ず確認を挟む。
///
/// シーンを進める・アプリを終わらせるといった実際の処理はここでは行わず、
/// GetResult() の結果を TitleScene が拾って動かす
/// </summary>
class TitleMenu
{
public:
	/// <summary>選ばれた結果。TitleScene が毎フレーム見る</summary>
	enum class Result {
		None,
		StartGame,
		/// <summary>
		/// トレーニングルームへ入る。1回の決定でそのまま移動し、
		/// 戦う相手は部屋の中の設定メニュー（TrainingMenu）で選ぶ
		/// </summary>
		StartTraining,
		Quit,
	};

	/// <summary>下敷きに使うテクスチャ。TitleScene の Initialize で読み込んでおくこと</summary>
	static void LoadTextures();

	void Initialize();

	/// <summary>メニューを開く。操作案内（PUSH A BUTTON）から切り替わるときに呼ぶ</summary>
	void Open();
	/// <summary>メニューを閉じる。ゲーム開始が決まったときなどに呼ぶ</summary>
	void Close();

	void Update();

	Result GetResult() const { return result_; }
	/// <summary>開いている（フェード中を含む）か</summary>
	bool IsOpen() const { return phase_ != Phase::Hidden; }

private:
	/// <summary>項目。並び順がそのまま画面の上からの順番になる</summary>
	enum class Item {
		GameStart,
		Training,
		Controls,
		Option,
		Quit,
	};

	enum class Phase {
		Hidden,
		Root,     // 一覧を操作している
		Controls, // 操作説明を開いている
		Option,   // 設定を開いている
		Confirm,  // 終了するか確認している
		Closing,  // 閉じきるのを待っている
	} phase_ = Phase::Hidden;

	// 一覧の操作
	void UpdateRoot();
	// 選ばれた項目を実行する
	void Decide();
	// 一覧の見た目を今の状態に合わせる
	void RefreshRoot();

	MenuNavigator navigator_{};
	MenuItemList itemList_{};
	ConfirmDialog confirmDialog_{};

	std::unique_ptr<ControlsPanel> controlsPanel_ = nullptr;
	std::unique_ptr<OptionPanel> optionPanel_ = nullptr;

	TextLabel* hint_ = nullptr; ///< 決定・戻るの操作案内

	Result result_ = Result::None;

	/// <summary>一覧の表示量。子パネルを開いている間は 0 まで引く</summary>
	float rootAlpha_ = 0.0f;

	// ==========================
	// レイアウト・演出パラメータ
	// ==========================
	// 項目が5つあるので、上はロゴの飾り罫、下は操作案内に挟まれる。
	// 間隔を少し詰めて、両方に触れない範囲へ収めている
	static constexpr float kItemStartY = 386.0f;
	static constexpr float kItemSpacing = 58.0f;
	static constexpr float kItemFontSize = 44.0f;
	static constexpr float kFadeTime = 0.22f;
};
