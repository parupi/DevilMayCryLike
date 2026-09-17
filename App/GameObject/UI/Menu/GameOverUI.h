#pragma once
#include "GameObject/UI/Common/MenuItemList.h"
#include "GameObject/UI/Common/MenuNavigator.h"

#include <Graphics/Rendering/Sprite/Sprite.h>
#include <Graphics/Text/TextLabel.h>

/// <summary>
/// 死んだときに出る画面。
///
/// これが無いと、死亡演出のあと問答無用でステージ頭から再開されてしまい、
/// 「もうやめる」を選ぶ余地が無い。やり直しとタイトルへ戻るを選ばせる。
///
/// シーンの切り替えはここでは行わず、GetResult() を GameSceneStateGameOver が拾って動かす
/// </summary>
class GameOverUI
{
public:
	enum class Result {
		None,
		Retry,   ///< ステージを最初からやり直す
		ToTitle, ///< タイトルへ戻る
	};

	void Initialize();

	/// <summary>表示を始める</summary>
	void Enter();

	void Update();

	Result GetResult() const { return result_; }

private:
	enum class Item {
		Retry,
		ToTitle,
	};

	MenuNavigator navigator_{};
	MenuItemList itemList_{};

	Sprite* backdrop_ = nullptr;
	TextLabel* title_ = nullptr;
	TextLabel* hint_ = nullptr;

	bool isActive_ = false;
	Result result_ = Result::None;
	float alpha_ = 0.0f;

	// ==========================
	// レイアウト
	// ==========================
	static constexpr float kTitleY = 280.0f;
	static constexpr float kItemStartY = 410.0f;
	static constexpr float kItemSpacing = 62.0f;
	static constexpr float kItemFontSize = 38.0f;
	static constexpr float kHintY = 630.0f;
	/// <summary>じわりと出したいので、他のメニューより長めにしてある</summary>
	static constexpr float kFadeTime = 0.8f;
	static constexpr float kBackdropAlpha = 0.75f;
};
