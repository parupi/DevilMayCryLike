#pragma once
#include "GameObject/UI/Common/MenuItemList.h"

#include <Graphics/Rendering/Sprite/Sprite.h>
#include <Graphics/Text/TextLabel.h>
#include <string>

class MenuNavigator;

/// <summary>
/// タイトルの TRAINING 画面。戦う敵を1種類選ぶ。
///
/// 並べる中身は EnemyCatalog が持っているので、敵を増やしてもここは変わらない。
/// 選ばれた結果を実際に反映する（GameSession に書いてシーンを進める）のは TitleMenu / TitleScene 側。
/// CONTROLS / OPTION と同じく、開閉のきっかけは TitleMenu が握る
/// </summary>
class TrainingSetupPanel
{
public:
	/// <summary>Update() の戻り。決定・キャンセルを持ち主へ返す</summary>
	enum class Result {
		None,
		Decided,  ///< 敵が決まった。GetSelectedEnemyClass() で取れる
		Canceled, ///< 戻るが押された
	};

	void Initialize();

	/// <summary>表示を始める</summary>
	void Open();
	/// <summary>閉じ始める</summary>
	void Close();

	Result Update(const MenuNavigator& navigator);

	/// <summary>まだ画面に映っているか（フェード中を含む）</summary>
	bool IsVisible() const { return state_ != State::Hidden; }

	/// <summary>選択中の敵のクラス名（Object3dFactory の登録キー）</summary>
	const std::string& GetSelectedEnemyClass() const;

private:
	enum class State {
		Hidden,
		Appearing,
		Shown,
		Closing,
	} state_ = State::Hidden;

	// 見た目を今の状態に合わせる
	void Refresh(float deltaTime);

	Sprite* backdrop_ = nullptr;      ///< 文字を読ませるための暗幕
	TextLabel* title_ = nullptr;
	TextLabel* description_ = nullptr; ///< 選択中の敵の説明
	TextLabel* hint_ = nullptr;

	MenuItemList itemList_{};

	float alpha_ = 0.0f;

	// ==========================
	// レイアウト
	// ==========================
	static constexpr float kItemStartY = 268.0f;
	static constexpr float kItemSpacing = 66.0f;
	static constexpr float kItemFontSize = 40.0f;
	static constexpr float kDescriptionY = 552.0f;

	static constexpr float kFadeTime = 0.22f;
	/// <summary>暗幕の濃さ。タイトルロゴはブルームで相当明るいので、かなり濃くしないと透けてくる</summary>
	static constexpr float kBackdropAlpha = 0.94f;
};
