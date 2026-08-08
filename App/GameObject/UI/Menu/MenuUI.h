#pragma once
#include "MenuDivider.h"
#include "GameObject/UI/Common/ConfirmDialog.h"
#include "GameObject/UI/Common/ControlsPanel.h"
#include "GameObject/UI/Common/MenuItemList.h"
#include "GameObject/UI/Common/MenuNavigator.h"
#include "GameObject/UI/Common/OptionPanel.h"

#include <Graphics/Text/TextLabel.h>
#include <memory>

class GameScene;

/// <summary>
/// ゲーム中のポーズメニュー。
///
/// CONTINUE / RETRY / CONTROLS / OPTION / TO TITLE の5項目。
/// RETRY と TO TITLE は進行が消えるので確認を挟む。
///
/// シーンの切り替えはここでは行わず、GetResult() を GameSceneStateMenu が拾って動かす
/// </summary>
class MenuUI
{
public:
	/// <summary>選ばれた結果。GameSceneStateMenu が毎フレーム見る</summary>
	enum class Result {
		None,
		Resume,  ///< ゲームへ戻る
		Retry,   ///< ステージを最初からやり直す
		ToTitle, ///< タイトルへ戻る
	};

	void Initialize(GameScene* scene);

	/// <summary>メニューを開く</summary>
	void Enter();
	/// <summary>メニューを閉じる（ゲームへ戻るときに GameSceneStateMenu が呼ぶ）</summary>
	void Exit();

	void Update();

	Result GetResult() const { return result_; }

private:
	/// <summary>項目。並び順がそのまま画面の上からの順番になる</summary>
	enum class Item {
		Continue,
		Retry,
		Controls,
		Option,
		ToTitle,
	};

	enum class Phase {
		Hidden,
		Root,     // 一覧を操作している
		Controls, // 操作説明を開いている
		Option,   // 設定を開いている
		Confirm,  // やり直し／タイトルへ戻るの確認中
		Closing,  // 閉じきるのを待っている
	} phase_ = Phase::Hidden;

	// 一覧の操作
	void UpdateRoot();
	// 選ばれた項目を実行する
	void Decide();
	// 一覧の見た目を今の状態に合わせる
	void RefreshRoot();

	GameScene* scene_ = nullptr;

	MenuNavigator navigator_{};
	MenuItemList itemList_{};
	ConfirmDialog confirmDialog_{};

	std::unique_ptr<MenuDivider> divider_ = nullptr;
	std::unique_ptr<ControlsPanel> controlsPanel_ = nullptr;
	std::unique_ptr<OptionPanel> optionPanel_ = nullptr;

	TextLabel* title_ = nullptr;
	TextLabel* hint_ = nullptr;

	Result result_ = Result::None;
	/// <summary>確認ダイアログで「はい」が出たときに確定する結果</summary>
	Result pendingResult_ = Result::None;

	/// <summary>一覧の表示量。子パネルを開いている間は 0 まで引く</summary>
	float rootAlpha_ = 0.0f;

	// ==========================
	// レイアウト
	// ==========================
	/// <summary>MenuDivider の飾り罫の間に収まるように置いている</summary>
	static constexpr float kTitleY = 268.0f;
	static constexpr float kItemStartY = 348.0f;
	static constexpr float kItemSpacing = 58.0f;
	static constexpr float kItemFontSize = 36.0f;
	static constexpr float kHintY = 622.0f;
	static constexpr float kFadeTime = 0.22f;
};
