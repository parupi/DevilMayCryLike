#pragma once
#include "ActionGuideTypes.h"

#include <array>
#include <string>

class Sprite;
class TextLabel;
struct ActionGuideSettings;

/// <summary>
/// ActionGuideViewModel を画面右下に描く（仕様書 §5.5 / §8）。
/// 位置・濃さ・フェード・アイコンだけを受け持ち、行動を出せるかどうかの判断はしない。
///
/// 1行の並び:  [組み合わせ "RB + ↑ +"] [アイコン] [押し方 "長押し"] [名前]
/// 行は上から優先度順。行の位置は固定の枠（最大件数ぶん）で、件数が減っても行は動かない。
/// 文字は中身が変わったときだけ作り直す（TextLabel::SetText が同じ文字なら何もしない）。
///
/// 絵のファイルは使わない。丸・角の丸み・帯は実行時にピクセルから作る（チュートリアルのパネルと同じ方式）。
/// スプライトは生成順に描かれるので、ポーズの暗幕より先に作ること
/// </summary>
class ActionGuideUI
{
public:
	void Initialize();

	/// <summary>
	/// 毎フレーム呼ぶ。
	/// </summary>
	/// <param name="viewModel">描く内容</param>
	/// <param name="allowed">操作中か。ポーズ・ゲームオーバー中は false にして即座に消す</param>
	/// <param name="raise">一番下の行を持ち上げる量（チュートリアルの部屋のスキップゲージを避ける）</param>
	/// <param name="deltaTime">実時間[秒]（ヒットストップやスローで遅れないように）</param>
	void Update(const ActionGuideViewModel& viewModel, const ActionGuideSettings& settings,
		bool allowed, float raise, float deltaTime);

private:
	struct Row {
		Sprite* strip = nullptr;    ///< 後ろの帯
		Sprite* dot = nullptr;      ///< 先行入力済みの印
		Sprite* circle = nullptr;   ///< 丸いアイコン
		Sprite* capLeft = nullptr;  ///< 横長アイコンの左端
		Sprite* middle = nullptr;   ///< 横長アイコンの中
		Sprite* capRight = nullptr; ///< 横長アイコンの右端（左端の反転）
		TextLabel* iconText = nullptr;
		TextLabel* prefix = nullptr;
		TextLabel* gesture = nullptr;
		TextLabel* name = nullptr;

		ActionGuideEntry entry;    ///< いま出している内容（消えていく途中も最後の内容を描く）
		bool hasEntry = false;     ///< 中身があるか（無ければ消えていく）
		float presence = 0.0f;     ///< 出ている度合い 0〜1。入れ替わるとフェードの始まりまで戻る
	};

	void ApplyRow(Row& row, int32_t index, const ActionGuideSettings& settings, float raise, bool usePad);
	void HideRow(Row& row);

	std::array<Row, 8> rows_{};
	/// <summary>ガイド全体の濃さ。表示・非表示を切り替えるときにフェードする</summary>
	float guideAlpha_ = 0.0f;
	bool allowed_ = false;
};
